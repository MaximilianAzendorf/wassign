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

#include "common.h"
#include "common_inputs.h"
#include "../src/Solution.h"
#include "../src/InputReader.h"
#include "../src/ConstraintParser.h"
#include "../src/ShotgunSolver.h"
#include "../src/Options.h"
#include "../src/OutputWriter.h"
#include "../src/Status.h"
#include "../src/ShotgunSolverThreaded.h"

#define PREFIX "[ShotgunSolverThreaded] "

Solution solve(const_ptr<InputData> data, int timeout = 1)
{
    auto options = default_options();
    options->set_timeout_seconds(timeout);

    ShotgunSolverThreaded solver(data, cs(data), sd(data), scoring(data, options), options);
    solver.start();

    Solution sol = solver.wait_for_result();

    Status::info("Iterations calculated: " + str(solver.progress().iterations));

    return sol;
}

TEST_CASE(PREFIX "Minimal")
{
    auto data = parse_data(INPUT_MINIMAL);
    auto solution = solve(data);
    expect_assignment(solution, "p,e");
    expect_scheduling(solution, "e,s");
}

TEST_CASE(PREFIX "Multiple slots with scheduling constraints")
{
    auto input = R"(
(slot) s1
(slot) s2
(event) e1: 3-3
(event) e2: 1-3
(event) e3: 2-3
(person) p1: 9 5 0
(person) p2: 5 9 5
(person) p3: 5 0 9
(constraint) slot of [e1] is [s1]
)";

    auto data = parse_data(input);
    auto solution = solve(data);
    expect_assignment(solution, "p1,e1,e2; p2,e1,e3; p3,e1,e3");
    expect_scheduling(solution, "e1,s1; e2,s2; e3,s2");
}