#pragma once

#include "Types.h"
#include "InputException.h"

#include <thread>

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
    inline static int _threadCount = (int)std::thread::hardware_concurrency();

    inline static string _timeoutStr;
    inline static string _csTimeoutStr;

    inline static const map<char, int> _timeMultiplier = {
            {'s', 1},
            {'m', 60},
            {'h', 60 * 60},
            {'d', 60 * 60 * 24},
            {'w', 60 * 60 * 24 * 7},
    };

    static auto parse_time(int& output);

public:
    static OptionsParseResult parse(int argc, char** argv, string const& header);

    static int verbosity();

    static vector<string> input_files();

    static string output_file();

    static int timeout_seconds();

    static int critical_set_timeout_seconds();

    static bool no_critical_sets();

    static bool no_stats();

    static bool ranked_preferences();

    static double preference_exponent();

    static bool any();

    static int thread_count();
};


