#pragma once

#include "Types.h"
#include "Util.h"
#include "Version.h"
#include "InputException.h"

#include <iostream>
#include <thread>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

enum OptionsParseResult
{
    OK,
    EXIT,
    ERROR
};

class Options
{
private:
    Options() = default;

    static po::variables_map varmap;

    inline static vector<string> _inputFiles;
    inline static string _outputFile;
    inline static int _verbosity = 3;
    inline static bool _any;
    inline static double _prefExp = 3.0;
    inline static bool _rankedPref;
    inline static int _timeout = 60;
    inline static int _csTimeout = 1;
    inline static bool _noCs;
    inline static bool _noStats;
    inline static int _threadCount = std::thread::hardware_concurrency();

    inline static string _timeoutStr;
    inline static string _csTimeoutStr;

    inline static const map<char, int> _timeMultiplier = {
            {'s', 1},
            {'m', 60},
            {'h', 60 * 60},
            {'d', 60 * 60 * 24},
            {'w', 60 * 60 * 24 * 7},
    };

    static auto parse_time(int& output)
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

public:
    static OptionsParseResult parse(int argc, char** argv, string const& header)
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
                    ("pref-exp,p", po::value(&_prefExp), "The preference exponent.]")
                    ("ranked-pref,r", po::bool_switch(&_rankedPref),
                     "Preferences of every participant will be transformed into a ranking.")
                    ("timeout,t", po::value(&_timeoutStr)->notifier(parse_time(_timeout)), "Sets the optimization timeout.")
                    ("cs-timeout,m", po::value(&_csTimeoutStr)->notifier(parse_time(_csTimeout)),
                     "Sets the timeout for attempting to satisfy critical sets of a certain preference level.")
                    ("no-cs", po::bool_switch(&_noCs), "Do not perform critical set analysis.")
                    ("no-stats", po::bool_switch(&_noStats), "Do not print solution statistics.")
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

    static int verbosity() { return _verbosity; }
    static vector<string> input_files() { return _inputFiles; }
    static string output_file() { return _outputFile; }
    static int timeout_seconds() { return _timeout; }
    static int critical_set_timeout_seconds() { return _csTimeout; }
    static bool no_critical_sets() { return _noCs; }
    static bool no_stats() { return _noStats; }
    static bool ranked_preferences() { return _rankedPref; }
    static double preference_exponent() { return _prefExp; }
    static bool any() { return _any; }
    static int thread_count() { return _threadCount; }
};


