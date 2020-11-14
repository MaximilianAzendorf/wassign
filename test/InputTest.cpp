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

#include "../src/InputReader.h"
#include "../src/SetData.h"
#include "../src/ConstraintParser.h"

#define PREFIX "[Input] "

TEST_CASE(PREFIX "Should parse everything without error")
{
    auto input = R"(
(set) .
(set) x
(set) a-b'c "d

(choice) w1: 0-9 [optional]
(choice)  w2:  1  - 178 [ optional, fixed p1 ]
(choice) w3 : 12-13 [12 parts, fixed p1, fixed p2, fixed p3]

(chooser) p1: 0 1 2
(chooser) p2: 12 100 1
(chooser) p5: 2 3 5

p3, 10100, 12, 0
p4, 20, 5, 21
)";

    auto data = InputReader::read_input(input);

    std::set<string> expectedSetNames = { ".", "x", "a-b'c \"d" };
    std::set<string> expectedChoiceNames = { "w1", "w2", "w3" };
    std::set<string> expectedChooserNames = { "p1", "p2", "p3", "p4", "p5" };

    for(auto const& s : data->sets()) expectedSetNames.erase(s.name());
    for(auto const& w : data->choices()) expectedChoiceNames.erase(w.name());
    for(auto const& p : data->choosers()) expectedChooserNames.erase(p.name());

    CHECK(data->set_count() == 4); // 3 real sets and 1 not-scheduled set.
    CHECK(data->choice_count() == 15); // 3 real choices + 11 extra parts of w2 + 1 unassigned set
    CHECK(data->chooser_count() == 5);

    CHECK(expectedChooserNames.empty());
    CHECK(expectedSetNames.empty());
    CHECK(expectedChoiceNames.empty());
}

TEST_CASE(PREFIX "Should create scheduling constraints for multi-part choices")
{
    auto input = R"(
(set) s1
(set) s2
(set) s3

(choice) e: 1-100 [3 parts]

(chooser) p: 1
)";

    auto data = InputReader::read_input(input);
    data->build_constraints(ConstraintParser::parse);

    CHECK(data->scheduling_constraints().size() == 6);

    for(auto const& c : data->scheduling_constraints())
    {
        if(c.type() != ConstraintType::ChoicesAreNotInSameSet)
        {
            REQUIRE(c.type() == ConstraintType::ChoicesHaveOffset);
            REQUIRE(c.right() - c.left() == c.extra());
        }
    }
}

TEST_CASE(PREFIX "Should create scheduling constraints for non-optional choices")
{
    auto input = R"(
(set) s1

(choice) e: 1-100
(choice) f: 1-100 [optional]

(chooser) p: 1 1
)";

    auto data = InputReader::read_input(input);
    data->build_constraints(ConstraintParser::parse);

    // One ChoiceIsNotInSet for e and one ChoiceIsInSet for the auto-generated unassigned-set-filling-choice.
    REQUIRE(data->scheduling_constraints().size() == 2);
}

TEST_CASE(PREFIX "Should auto-generate set if none is given")
{
    auto input = R"(
(choice) w1: 1-100
(chooser) p1: 0
)";

    auto data = InputReader::read_input(input);

    REQUIRE(data->set_count() == 1);
}

TEST_CASE(PREFIX "Should not accept too many preferences")
{
    auto input = R"(
(choice) w1: 0-1
(chooser) p1: 0 1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept too few preferences")
{
    auto input = R"(
(choice) w1: 0-1
(choice) w2: 0-1
(chooser) p1: 0
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept choices with zero minimum chooser count")
{
    auto input = R"(
(set) s
(choice) e: 0-100
(chooser) p: 1 1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept input without choices")
{
    auto input = R"(
(set) s1
(chooser) p1:
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept input without choosers")
{
    auto input = R"(
(set) s1
(choice) w1: 0-1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}