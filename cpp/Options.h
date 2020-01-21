#pragma once

#include "Types.h"

class Options
{
private:
    Options() = default;

public:
    static void parse(int argc, char** argv);

    static int verbosity();
    static vector<string> input_files();
    static string output_file();
    static bool csv_output();
    static bool show_help();
    static int timeout_seconds();
    static int critical_set_timeout_seconds();
    static bool no_critical_sets();
    static bool no_stats();
    static bool ranked_preferences();
    static double preference_exponent();
    static bool any();
    static int thread_count();
};


