#pragma once

#include "Solution.h"

class OutputWriter
{
private:
    OutputWriter() = default;

public:
    static string write_scheduling_solution(Solution const& solution);

    static string write_assignment_solution(Solution const& solution);
};


