#pragma once

#include <iostream>

#include "Types.h"
#include "Options.h"

class Status
{
private:
    Status() = default;

    static string color(int foregroundColor)
    {
        std::stringstream s;
        s << "\033[";
        s << foregroundColor;
        s << "m";

        return s.str();
    }

    static string color_reset()
    {
        return "\033[0m";
    }

public:
    static void info(string const& text)
    {
        if(Options::verbosity() >= 3)
        {
            std::cerr << "INFO:    " << text << std::endl;
        }
    }

    static void info_important(string const& text)
    {
        if(Options::verbosity() >= 1)
        {
            std::cerr << "INFO:    " << text << std::endl;
        }
    }

    static void warning(string const& text)
    {
        if(Options::verbosity() >= 2)
        {
            std::cerr << color(33) << "WARNING: " << text << color_reset() << std::endl;
        }
    }

    static void error(string const& text)
    {
        if(Options::verbosity() >= 1)
        {
            std::cerr << color(31) << "ERROR:   " << text << color_reset() << std::endl;
        }
    }
};


