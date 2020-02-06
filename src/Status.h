#pragma once

#include "Types.h"
#include "Options.h"

class Status
{
private:
    Status() = default;

    static string color(int foregroundColor);

    static string color_reset();

    static bool _output;

public:
    static void enable_output();

    static void info(string const& text);

    static void info_important(string const& text);

    static void warning(string const& text);

    static void error(string const& text);
};


