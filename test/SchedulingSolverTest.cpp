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
#include "../src/SchedulingSolver.h"
#include <cstdio>

#define PREFIX "[SchedulingSolver] "

void common_check(const_ptr<Scheduling> scheduling)
{
    int s1 = 0;
    int s2 = 0;
    for(int w = 0; w < 4; w++)
    {
        (scheduling->slot_of(w) == 0 ? s1 : s2)++;
    }

    REQUIRE(s1 == 2);
    REQUIRE(s2 == 2);
}

TEST_CASE(PREFIX "Minimal")
{
    Rng::seed(12);

    auto data = parse_data(INPUT_MINIMAL);
    SchedulingSolver solver(data, csa(data, false), default_options());

    REQUIRE(solver.next_scheduling());
    auto scheduling = solver.scheduling();

    expect_scheduling(sol(scheduling, std::make_shared<Assignment>(data, vector<vector<int>>{vector<int>{0}})), "e,s");
}

TEST_CASE(PREFIX "Works without constraints")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));

var p = [1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());
    REQUIRE(solver.next_scheduling());
    auto scheduling = solver.scheduling();

    common_check(scheduling);
}

TEST_CASE(PREFIX "Constraint ChoiceIsInSlot works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));

var p = [1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(choice("c1").slot == slot("s1"));
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        REQUIRE(scheduling->slot_of(0) == 0);
        common_check(scheduling);
    }
}

TEST_CASE(PREFIX "Constraint ChoiceIsNotInSlot works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));

var p = [1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(choice("c1").slot != slot("s1"));
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        REQUIRE(scheduling->slot_of(0) != 0);
        common_check(scheduling);
    }
}

TEST_CASE(PREFIX "Constraint ChoicesAreInSameSlot works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));

var p = [1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(choice("c1").slot == choice("c3").slot);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        REQUIRE(scheduling->slot_of(0) == scheduling->slot_of(2));
        common_check(scheduling);
    }
}

TEST_CASE(PREFIX "Constraint ChoicesAreNotInSameSlot works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));

var p = [1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(choice("c1").slot != choice("c3").slot);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        REQUIRE(scheduling->slot_of(0) != scheduling->slot_of(2));
        common_check(scheduling);
    }
}

TEST_CASE(PREFIX "Constraint ChoicesHaveOffset works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2), parts(2));

var p = [1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        REQUIRE(scheduling->slot_of(2) == 0);
        REQUIRE(scheduling->slot_of(3) == 1);
        common_check(scheduling);
    }
}

TEST_CASE(PREFIX "Constraint SlotHasLimitedSize (==) works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

var p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(slot("s1").size == 2);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        int s1 = 0;
        int s2 = 0;
        for(int w = 0; w < data->choice_count(); w++)
        {
            (scheduling->slot_of(w) == 0 ? s1 : s2)++;
        }

        REQUIRE(s1 == 2);
    }
}

TEST_CASE(PREFIX "Constraint SlotHasLimitedSize (!=) works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

var p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(slot("s1").size != 2);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        int s1 = 0;
        int s2 = 0;
        for(int w = 0; w < data->choice_count(); w++)
        {
            (scheduling->slot_of(w) == 0 ? s1 : s2)++;
        }

        REQUIRE(s1 != 2);
    }
}

TEST_CASE(PREFIX "Constraint SlotHasLimitedSize (<) works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

var p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(slot("s1").size < 3);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        int s1 = 0;
        int s2 = 0;
        for(int w = 0; w < data->choice_count(); w++)
        {
            (scheduling->slot_of(w) == 0 ? s1 : s2)++;
        }

        REQUIRE(s1 < 3);
    }
}

TEST_CASE(PREFIX "Constraint SlotHasLimitedSize (<=) works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

var p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(slot("s1").size <= 2);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        int s1 = 0;
        int s2 = 0;
        for(int w = 0; w < data->choice_count(); w++)
        {
            (scheduling->slot_of(w) == 0 ? s1 : s2)++;
        }

        REQUIRE(s1 <= 2);
    }
}

TEST_CASE(PREFIX "Constraint SlotHasLimitedSize (>) works")
{
    Rng::seed(12);

    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

var p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(slot("s1").size > 2);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        int s1 = 0;
        int s2 = 0;
        for(int w = 0; w < data->choice_count(); w++)
        {
            (scheduling->slot_of(w) == 0 ? s1 : s2)++;
        }

        REQUIRE(s1 > 2);
    }
}

TEST_CASE(PREFIX "Constraint SlotHasLimitedSize (>=) works")
{
    Rng::seed(12);
    
    auto data = parse_data(R"(
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

var p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

+constraint(slot("s1").size >= 3);
)");

    SchedulingSolver solver(data, csa(data, false), default_options());

    for(int i = 0; i < 16; i++)
    {
        REQUIRE(solver.next_scheduling());
        auto scheduling = solver.scheduling();

        int s1 = 0;
        int s2 = 0;
        for(int w = 0; w < data->choice_count(); w++)
        {
            (scheduling->slot_of(w) == 0 ? s1 : s2)++;
        }

        REQUIRE(s1 >= 3);
    }
}