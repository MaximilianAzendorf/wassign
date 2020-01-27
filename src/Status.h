#pragma once

#include "Types.h"
#include "Options.h"

class Status
{
private:
    Status() = default;

    static string color(int foregroundColor);

    static string color_reset();

public:
    static void info(string const& text);

    static void info_important(string const& text);

    static void warning(string const& text);

    static void error(string const& text);
};


