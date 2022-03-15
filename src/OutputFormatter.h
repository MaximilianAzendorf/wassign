/*
 * Copyright 2020 Maximilian Azendorf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Solution.h"
#include <iostream>

/**
 * Formats the given solution into CSV data.
 */
class OutputFormatter
{
private:
    OutputFormatter() = default;

public:
    static string write_scheduling_solution(Solution const& solution);

    static string write_assignment_solution(Solution const& solution);
};


