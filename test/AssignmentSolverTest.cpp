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
#include "inputs/minimal.h"
#include "inputs/big_realistic.h"
#include "../src/AssignmentSolver.h"
#include <cstdio>

#define PREFIX "[AssignmentSolver] "

TEST_CASE(PREFIX "Minimal")
{
    auto data = parse_data(INPUT_MINIMAL);
    AssignmentSolver solver(data, csa(data), sd(data), default_options());

    auto scheduling = MAKE_SCHED(data, 0);
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p,e");
}

TEST_CASE(PREFIX "Large")
{
    auto data = parse_data(INPUT_BIG_REALISTIC);
    AssignmentSolver solver(data, csa(data, false), sd(data), default_options());

    auto scheduling = MAKE_SCHED(data, SCHEDULING_BIG_REALISTIC);
    auto assignment = solver.solve(scheduling);
    auto solution = sol(scheduling, assignment);

    REQUIRE(Scoring(data, default_options()).is_feasible(solution));
    expect_assignment(solution, ASSIGNMENT_STR_BIG_REALISTIC);
}

TEST_CASE(PREFIX "Works without constraints")
{
    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 0, 100, 50]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [0, 100, 70, 100]);
)");

    AssignmentSolver solver(data, csa(data, false), sd(data), default_options());
    auto scheduling = MAKE_SCHED(data, (vector<int> {0, 0, 1, 1}));
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p1,c1,c3;p2,c1,c3;p3,c2,c4;p4,c2,c4");
}

TEST_CASE(PREFIX "Constraint ChoicesHaveSameChoosers works")
{
    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 0, 100, 50]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [0, 100, 70, 100]);

+constraint(choice("c1").choosers == choice("c4").choosers);
)");

    AssignmentSolver solver(data, csa(data, false), sd(data), default_options());
    auto scheduling = MAKE_SCHED(data, (vector<int> {0, 0, 1, 1}));
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p1,c1,c4;p2,c1,c4;p3,c2,c3;p4,c2,c3");
}

TEST_CASE(PREFIX "Constraint ChooserIsInChoice works")
{
    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 0, 100, 50]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [0, 100, 70, 100]);

+constraint(chooser("p1").choices.contains(choice("c4")));
)");

    AssignmentSolver solver(data, csa(data, false), sd(data), default_options());
    auto scheduling = MAKE_SCHED(data, (vector<int> {0, 0, 1, 1}));
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p1,c1,c4;p2,c1,c3;p3,c2,c4;p4,c2,c3");
}

TEST_CASE(PREFIX "Constraint ChooserIsNotInChoice works")
{
    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 0, 100, 50]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [0, 100, 70, 100]);

+constraint(chooser("p1").choices.contains_not(choice("c3")));
)");

    AssignmentSolver solver(data, csa(data, false), sd(data), default_options());
    auto scheduling = MAKE_SCHED(data, (vector<int> {0, 0, 1, 1}));
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p1,c1,c4;p2,c1,c3;p3,c2,c4;p4,c2,c3");
}

TEST_CASE(PREFIX "Constraint ChoosersHaveSameChoices works")
{
    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 70, 100, 70]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [70, 100, 70, 100]);

+constraint(chooser("p1").choices == chooser("p4").choices);
)");

    AssignmentSolver solver(data, csa(data, false), sd(data), default_options());
    auto scheduling = MAKE_SCHED(data, (vector<int> {0, 0, 1, 1}));
    auto assignment = solver.solve(scheduling);

    expect_assignment(sol(scheduling, assignment), "p1,c1,c3;p2,c2,c4;p3,c2,c4;p4,c1,c3");
}