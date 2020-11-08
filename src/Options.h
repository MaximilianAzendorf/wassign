#pragma once

#include "Types.h"
#include "InputException.h"

#include <thread>

enum OptionsParseStatus
{
    OK,
    EXIT,
    ERROR
};

class Options
{
private:
    vector<string> _inputFiles;
    string _outputFile;
    int _verbosity = 1;
    bool _any = false;
    double _prefExp = 3.0;
    int _timeout = 60;
    int _csTimeout = 3;
    bool _noCs = false;
    int _threadCount = (int)std::thread::hardware_concurrency();

    string _timeoutStr;
    string _csTimeoutStr;

    inline static const map<char, int> _timeMultiplier = {
            {'s', 1},
            {'m', 60},
            {'h', 60 * 60},
            {'d', 60 * 60 * 24},
            {'w', 60 * 60 * 24 * 7},
    };

    static int parse_time(string value);

public:
    Options() = default;

    static OptionsParseStatus parse(int argc, char** argv, string const& header, Options& result);

    static Options default_options();

    [[nodiscard]] int verbosity() const;

    [[nodiscard]] vector<string> input_files() const;

    [[nodiscard]] string output_file() const;

    [[nodiscard]] int timeout_seconds() const;

    [[nodiscard]] int critical_set_timeout_seconds() const;

    [[nodiscard]] bool no_critical_sets() const;

    [[nodiscard]] double preference_exponent() const;

    [[nodiscard]] bool any() const;

    [[nodiscard]] int thread_count() const;

    void set_verbosity(int verbosity);

    void set_input_files(vector<string> inputFiles);

    void set_output_file(string outputFile);

    void set_timeout_seconds(int timeoutSeconds);

    void set_critical_set_timeout_seconds( int csTimeoutSeconds);

    void set_no_critical_sets(bool noCriticalSets);

    void set_preference_exponent(double prefExponent);

    void set_any(bool any);

    void set_thread_count(int threadCount);
};


