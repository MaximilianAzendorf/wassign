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

#include "../src/input/InputReader.h"
#include "../src/SlotData.h"
#include "../src/input/ConstraintBuilder.h"
#include "../src/input/InputReader.h"

#define PREFIX "[Input] "

TEST_CASE(PREFIX "Should parse everything without error")
{
    auto input = R"(
+slot(".");
+slot("x");
+slot("a-b'c d");

+choice("w1", bounds(1, 9), optional);
+choice("w2", min(1), max(178), optional_if(false));
+choice("w3", bounds(12, 13), parts(3));

+chooser("p1", [0, 1, 2]);
+chooser("p2", [12, 100, 1]);
+chooser("p3", [10100, 12, 0]);
+chooser("p4", [20, 5, 21]);
+chooser("q5", [2, 4, 5]);

chooser("q5").name = "p5";
)";

    auto reader = InputReader(Options::default_options());
    auto data = reader.read_input(input);

    std::set<string> expectedSlotNames = {".", "x", "a-b'c d"};
    std::set<string> expectedChoiceNames = {"w1", "w2", "w3"};
    std::set<string> expectedChooserNames = {"p1", "p2", "p3", "p4", "p5"};

    SECTION("Should have the correct number of slots, choices and choosers")
    {
        REQUIRE(data->slot_count() == 4); // 3 real slots and 1 not-scheduled slot.
        REQUIRE(data->choice_count() == 6); // 3 real choices + 2 extra parts of w2 + 1 unassigned slot
        REQUIRE(data->chooser_count() == 5);
    }

    SECTION("Should have the correct slots, choices and choosers")
    {
        for (auto const& s : data->slots()) expectedSlotNames.erase(s.name);
        for (auto const& w : data->choices()) expectedChoiceNames.erase(w.name);
        for (auto const& p : data->choosers()) expectedChooserNames.erase(p.name);

        REQUIRE(expectedChooserNames.empty());
        REQUIRE(expectedSlotNames.empty());
        REQUIRE(expectedChoiceNames.empty());
    }

    SECTION("Should have the correct number of constraints")
    {
        REQUIRE(data->scheduling_constraints().size() == 9);
        REQUIRE(data->assignment_constraints().size() == 3);
    }
}

TEST_CASE(PREFIX "Should create scheduling constraints for multi-part choices")
{
    auto input = R"(
+slot("s1");
+slot("s2");
+slot("s3");

+choice("e", bounds(1, 100), parts(3));

+chooser("p", [1]);
)";

    auto data = InputReader(Options::default_options()).read_input(input);

    CHECK(data->scheduling_constraints().size() == 6);

    for(auto const& c : data->scheduling_constraints())
    {
        if(c.type() != ConstraintType::ChoicesAreNotInSameSlot)
        {
            REQUIRE(c.type() == ConstraintType::ChoicesHaveOffset);
            REQUIRE(c.right() - c.left() == c.extra());
        }
        else
        {
            REQUIRE(c.type() == ChoicesAreNotInSameSlot);
        }
    }
}

TEST_CASE(PREFIX "Should create scheduling constraints for non-optional choices")
{
    auto input = R"(
+slot("s1");

+choice("e", bounds(1, 100));
+choice("f", bounds(1, 100), optional);

+chooser("p", [1, 1]);
)";

    auto data = InputReader(Options::default_options()).read_input(input);

    // One ChoiceIsNotInSlot for e and one ChoiceIsInSlot for the auto-generated unassigned-slot-filling-choice.
    REQUIRE(data->scheduling_constraints().size() == 2);
}

TEST_CASE(PREFIX "Should auto-generate set if none is given")
{
    auto input = R"(
+choice("w1", bounds(1, 100));
+chooser("p1", [0]);
)";

    auto data = InputReader(Options::default_options()).read_input(input);

    REQUIRE(data->slot_count() == 1);
}

TEST_CASE(PREFIX "Should not accept too many preferences")
{
    auto input = R"(
+choice("w1", max(1));
+chooser("p1", [0, 1]);
)";

    REQUIRE_THROWS_AS(InputReader(Options::default_options()).read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept too few preferences")
{
    auto input = R"(
+choice("w1", max(1));
+choice("w2", max(1));
+chooser("p1", [0]);
)";

    REQUIRE_THROWS_AS(InputReader(Options::default_options()).read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept choices with zero minimum chooser count")
{
    auto input = R"(
+slot("s")
+choice("e", max(100));
+chooser("p", [1, 1]);
)";

    REQUIRE_THROWS_AS(InputReader(Options::default_options()).read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept input without choices")
{
    auto input = R"(
+slot("s1");
+chooser("p1", []);
)";

    REQUIRE_THROWS_AS(InputReader(Options::default_options()).read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept input without choosers")
{
    auto input = R"(
+slot("s1");
+choice("w1", max(1));
)";

    REQUIRE_THROWS_AS(InputReader(Options::default_options()).read_input(input), InputException);
}

TEST_CASE(PREFIX "Should parse slots")
{
    auto input = R"(
+slot("a");
+slot("b");

+choice("x", bounds(1, 1));
+chooser("y", [0]);
)";

    auto reader = InputReader(Options::default_options());
    auto data = reader.read_input(input);

    REQUIRE(data->slot_count() == 2);
    REQUIRE(data->slot(0).name == "a");
    REQUIRE(data->slot(1).name == "b");
}

TEST_CASE(PREFIX "Should parse choices")
{
    auto input = R"(
+choice("a");
+choice("b", bounds(2, 3), optional);
+choice("c", bounds(5, 7), parts(2));

+chooser("y", [11, 13, 17]);
)";

    auto reader = InputReader(Options::default_options());
    auto data = reader.read_input(input);

    REQUIRE(data->choice_count() > 4); // 3 normal, 1 extra part,  at least one unassigned

    REQUIRE(data->choice(0).name == "a");
    REQUIRE(data->choice(0).min == 1);
    REQUIRE(data->choice(0).max == 1);
    REQUIRE(data->choice(0).has_continuation() == false);

    REQUIRE(data->choice(1).name == "b");
    REQUIRE(data->choice(1).min == 2);
    REQUIRE(data->choice(1).max == 3);
    REQUIRE(data->choice(1).has_continuation() == false);

    REQUIRE(data->choice(2).name[0] == 'c');
    REQUIRE(data->choice(2).min == 5);
    REQUIRE(data->choice(2).max == 7);
    REQUIRE(data->choice(2).has_continuation() == true);
    REQUIRE(data->choice(2).continuation_value() == 3);

    //REQUIRE(data->choice(3).name[0] == "..."); // name is not relevant
    REQUIRE(data->choice(3).min == 5);
    REQUIRE(data->choice(3).max == 7);
    REQUIRE(data->choice(3).has_continuation() == false);

    REQUIRE(data->chooser(0).preferences[0] == 6); // preferences get reversed and normalized.
    REQUIRE(data->chooser(0).preferences[1] == 4);
    REQUIRE(data->chooser(0).preferences[2] == 0);
    REQUIRE(data->chooser(0).preferences[3] == 0);
    REQUIRE(data->chooser(0).preferences[4] > 0);
}

TEST_CASE(PREFIX "Should parse choosers")
{
    auto input = R"(
+choice("x");
+choice("y");

+chooser("a", [2, 3]);
+chooser("b", [5, 7]);
)";

    auto reader = InputReader(Options::default_options());
    auto data = reader.read_input(input);

    REQUIRE(data->chooser(0).name == "a");
    REQUIRE(data->chooser(0).preferences == vector<int>{5, 4}); // preferences get reversed and normalized

    REQUIRE(data->chooser(1).name == "b");
    REQUIRE(data->chooser(1).preferences == vector<int>{2, 0});
}

TEST_CASE(PREFIX "Should parse constraints")
{
    auto input = R"(
var s1 = +slot("s1");
var s2 = +slot("s2");

var w1 = +choice("a");
var w2 = +choice("b");

var p1 = +chooser("c", [0, 0]);
var p2 = +chooser("d", [0, 0]);

+constraint(w2.slot == s2);
+constraint(w2.slot != s2);
+constraint(w2.slot == w1.slot);
+constraint(w1.slot != w2.slot);
+constraint(s2.size == 1);
+constraint(s2.size <= 3);

// reduced ones
+constraint(s2.choices.contains(w1));
+constraint(s1.choices.contains_not(w2));
+constraint(s2.choices == s2.choices); // tautology, will be removed

+constraint(w2.choosers == w1.choosers);
+constraint(p2.choices.contains(w2));
+constraint(p2.choices.contains_not(w2));
+constraint(p2.choices == p1.choices);

// reduced ones
+constraint(w2.choosers.contains(p1));
+constraint(w1.choosers.contains_not(p2));
)";

    auto reader = InputReader(Options::default_options());
    auto data = reader.read_input(input);

    auto con = data->scheduling_constraints();
    auto ac = data->assignment_constraints();
    con.insert(con.end(), ac.begin(), ac.end());

    auto required = set<Constraint> {
            Constraint(ChoiceIsInSlot, 0, 1),
            Constraint(ChoiceIsInSlot, 1, 1),
            Constraint(ChoiceIsNotInSlot, 1, 0),
            Constraint(ChoicesAreInSameSlot, 1, 0),
            Constraint(ChoicesAreNotInSameSlot, 0, 1),
            Constraint(SlotHasLimitedSize, 1, 1, Eq),
            Constraint(SlotHasLimitedSize, 1, 3, Leq),
            Constraint(ChoicesHaveSameChoosers, 1, 0),
            Constraint(ChooserIsInChoice, 0, 1),
            Constraint(ChooserIsInChoice, 1, 1),
            Constraint(ChooserIsNotInChoice, 1, 0),
            Constraint(ChooserIsNotInChoice, 1, 1),
            Constraint(ChoosersHaveSameChoices, 1, 0)
    };

    for(auto c : con)
    {
        auto reqIt = required.find(c);
        if(reqIt != required.end())
        {
            required.erase(reqIt);
        }
    }

    REQUIRE(required.empty());
}