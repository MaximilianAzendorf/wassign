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
#include "../src/AssignmentSolver.h"

#define PREFIX "[AssignmentSolver] "

TEST_CASE(PREFIX "Minimal")
{
    auto data = parse_data(INPUT_MINIMAL);
    AssignmentSolver solver(data, cs(data), sd(data), default_options());

    auto scheduling = MAKE_SCHED(data, 0);
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p,e");
}

TEST_CASE(PREFIX "Large")
{
    auto data = parse_data(INPUT_BIG_REALISTIC);
    AssignmentSolver solver(data, cs(data, false), sd(data), default_options());

    auto scheduling = MAKE_SCHED(data, SCHEDULING_BIG_REALISTIC);
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), ASSIGNMENT_STR_BIG_REALISTIC);
}