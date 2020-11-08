#include "Options.h"

#include "Util.h"
#include "Version.h"
#include "InputException.h"

#include <iostream>
#include <boost/program_options.hpp>
#include <utility>

namespace po = boost::program_options;

auto Options::parse_time(int& output)
{
    return [&](string const& value)
    {
        int time = 0;
        int current = 0;

        for(char c : value)
        {
            if(c >= '0' && c <= '9')
            {
                current = current * 10 + (c - '0');
            }
            else if(c >= 'a' && c <= 'z')
            {
                time += current * _timeMultiplier.at(c);
            }
            else
            {
                throw InputException("Unknown time specifier " + value + ".");
            }
        }

        output = time;
    };
}

OptionsParseStatus Options::parse(int argc, char **argv, string const& header, Options& result)
{
    po::options_description desc;

    try
    {
        desc.add_options()
                ("help,h", "Show this help.")
                ("version", "Show version.")
                ("input,i", po::value(&result._inputFiles)->multitoken(), "Specifies an input file.")
                ("output,o", po::value(&result._outputFile), "Specifies an output file name.")
                ("verbosity,v", po::value(&result._verbosity),
                 "A number between 0 and 3 indicating how much status information should be given.")
                ("any,a", po::bool_switch(&result._any), "Stop after the first found solution.")
                ("pref-exp,p", po::value(&result._prefExp), "The preference exponent.")
                ("timeout,t", po::value(&result._timeoutStr)->notifier(parse_time(result._timeout)), "Sets the optimization timeout.")
                ("cs-timeout,m", po::value(&result._csTimeoutStr)->notifier(parse_time(result._csTimeout)),
                 "Sets the timeout for attempting to satisfy critical sets of a certain preference level.")
                ("no-cs", po::bool_switch(&result._noCs), "Do not perform critical set analysis.")
                ("threads,j", po::value(&result._threadCount), "Number of threads to use for computation.");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help") || argc == 1)
        {
            std::cout << header << std::endl << desc << std::endl;
            return EXIT;
        } else if (vm.count("version"))
        {
            std::cout << WSOLVE_VERSION << std::endl;
            return EXIT;
        }
    }
    catch(po::error const& ex)
    {
        return ERROR;
    }

    if(result.verbosity() > 0)
    {
        std::cerr << header << std::endl;
    }
    return OK;
}

Options Options::default_options()
{
    return Options();
}

int Options::verbosity() const
{
    return _verbosity;
}

vector<string> Options::input_files() const
{
    return _inputFiles;
}

string Options::output_file() const
{
    return _outputFile;
}

int Options::timeout_seconds() const
{
    return _timeout;
}

int Options::critical_set_timeout_seconds() const
{
    return _csTimeout;
}

bool Options::no_critical_sets() const
{
    return _noCs;
}

double Options::preference_exponent() const
{
    return _prefExp;
}

bool Options::any() const
{
    return _any;
}

int Options::thread_count() const
{
    return _threadCount;
}

void Options::set_verbosity(int verbosity)
{
    _verbosity = verbosity;
}

void Options::set_input_files(vector<string> inputFiles)
{
    _inputFiles = std::move(inputFiles);
}

void Options::set_no_critical_sets(bool noCriticalSets)
{
    _noCs = noCriticalSets;
}

void Options::set_output_file(string outputFile)
{
    _outputFile = outputFile;
}

void Options::set_timeout_seconds(int timeoutSeconds)
{
    _timeout = timeoutSeconds;
}

void Options::set_critical_set_timeout_seconds(int csTimeoutSeconds)
{
    _csTimeout = csTimeoutSeconds;
}

void Options::set_preference_exponent(double prefExponent)
{
    _prefExp = prefExponent;
}

void Options::set_any(bool any)
{
    _any = any;
}

void Options::set_thread_count(int threadCount)
{
    _threadCount = threadCount;
}
