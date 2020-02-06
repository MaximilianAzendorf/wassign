#include "common.h"

#include "../src/InputReader.h"
#include "../src/SlotData.h"

TEST_CASE("Should parse everything correctly", INTEGRATION)
{
    auto input = R"(
(slot) .
(slot) x
(slot) a-b'c"d

(event) w1: 0-9 [optional]
(event)  w2:  1  - 178 [ optional, fixed p1 ]
(event) w3 : 12-13 [12 parts, fixed p1, fixed p2, fixed p3]

(person) p1: 0 1 2
(person) p2: 12 100 1

p3, 10100, 12, 0
)";

    auto data = InputReader::read_input(input);

    std::set<string> expectedSlotNames = { ".", "x", "a-b'c\"d" };
    std::set<string> expectedWorkshopNames = { "w1", "w2", "w3" };
    std::set<string> expectedPersonNames = { "p1", "p2", "p3" };

    for(auto const& s : data.slots()) expectedSlotNames.erase(s.name());
    for(auto const& w : data.workshops()) expectedWorkshopNames.erase(w.name());
    for(auto const& p : data.participants()) expectedPersonNames.erase(p.name());

    REQUIRE(data.slot_count() == 4); // 3 real slots and 1 not-scheduled slot.
    REQUIRE(data.workshop_count() == 15); // 3 real workshops + 11 extra parts of w2 + 1 unassigned slot
    REQUIRE(data.participant_count() == 3);

    REQUIRE(expectedPersonNames.empty());
    REQUIRE(expectedSlotNames.empty());
    REQUIRE(expectedWorkshopNames.empty());
}

TEST_CASE("Should auto-generate slot if none is given", INTEGRATION)
{
    auto input = R"(
(event) w1: 0-1
(person) p1: 0
)";

    auto data = InputReader::read_input(input);

    REQUIRE(data.slot_count() == 1);
}

TEST_CASE("Should not accept too many preferences", INTEGRATION)
{
    auto input = R"(
(event) w1: 0-1
(person) p1: 0 1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE("Should not accept too few preferences", INTEGRATION)
{
    auto input = R"(
(event) w1: 0-1
(event) w2: 0-1
(person) p1: 0
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE("Should not accept input without events", INTEGRATION)
{
    auto input = R"(
(slot) s1
(person) p1:
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE("Should not accept input without participants", INTEGRATION)
{
    auto input = R"(
(slot) s1
(event) w1: 0-1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}