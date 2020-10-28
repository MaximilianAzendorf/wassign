#include "Options.h"

#include "Util.h"
#include "Version.h"
#include "InputException.h"

#include <iostream>
#include <boost/program_options.hpp>

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

OptionsParseResult Options::parse(int argc, char **argv, string const& header)
{
    po::options_description desc;

    try
    {
        desc.add_options()
                ("help,h", "Show this help")
                ("version", "Show version.")
                ("input,i", po::value(&_inputFiles)->multitoken(), "Specifies an input file.")
                ("output,o", po::value(&_outputFile), "Specifies an output file name.")
                ("verbosity,v", po::value(&_verbosity),
                 "A number between 0 and 3 indicating how much status information should be given.")
                ("any,a", po::bool_switch(&_any), "Stop after the first found solution.")
                ("pref-exp,p", po::value(&_prefExp), "The preference exponent.")
                ("timeout,t", po::value(&_timeoutStr)->notifier(parse_time(_timeout)), "Sets the optimization timeout.")
                ("cs-timeout,m", po::value(&_csTimeoutStr)->notifier(parse_time(_csTimeout)),
                 "Sets the timeout for attempting to satisfy critical sets of a certain preference level.")
                ("no-cs", po::bool_switch(&_noCs), "Do not perform critical set analysis.")
                ("threads,j", po::value(&_threadCount), "Number of threads to use for computation.");

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

    if(verbosity() > 0)
    {
        std::cerr << header << std::endl;
    }
    return OK;
}

int Options::verbosity()
{
    return _verbosity;
}

vector<string> Options::input_files()
{
    return _inputFiles;
}

string Options::output_file()
{
    return _outputFile;
}

int Options::timeout_seconds()
{
    return _timeout;
}

int Options::critical_set_timeout_seconds()
{
    return _csTimeout;
}

bool Options::no_critical_sets()
{
    return _noCs;
}

double Options::preference_exponent()
{
    return _prefExp;
}

bool Options::any()
{
    return _any;
}

int Options::thread_count()
{
    return _threadCount;
}
