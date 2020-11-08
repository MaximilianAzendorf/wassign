#include "common.h"

#include "../src/InputReader.h"
#include "../src/SlotData.h"
#include "../src/ConstraintParser.h"

#define PREFIX "[Input] "

TEST_CASE(PREFIX "Should parse everything without error")
{
    auto input = R"(
(slot) .
(slot) x
(slot) a-b'c "d

(event) w1: 0-9 [optional]
(event)  w2:  1  - 178 [ optional, fixed p1 ]
(event) w3 : 12-13 [12 parts, fixed p1, fixed p2, fixed p3]

(person) p1: 0 1 2
(person) p2: 12 100 1
(person) p5: 2 3 5

p3, 10100, 12, 0
p4, 20, 5, 21
)";

    auto data = InputReader::read_input(input);

    std::set<string> expectedSlotNames = { ".", "x", "a-b'c \"d" };
    std::set<string> expectedWorkshopNames = { "w1", "w2", "w3" };
    std::set<string> expectedPersonNames = { "p1", "p2", "p3", "p4", "p5" };

    for(auto const& s : data.slots()) expectedSlotNames.erase(s.name());
    for(auto const& w : data.workshops()) expectedWorkshopNames.erase(w.name());
    for(auto const& p : data.participants()) expectedPersonNames.erase(p.name());

    CHECK(data.slot_count() == 4); // 3 real slots and 1 not-scheduled slot.
    CHECK(data.workshop_count() == 15); // 3 real workshops + 11 extra parts of w2 + 1 unassigned slot
    CHECK(data.participant_count() == 5);

    CHECK(expectedPersonNames.empty());
    CHECK(expectedSlotNames.empty());
    CHECK(expectedWorkshopNames.empty());
}

TEST_CASE(PREFIX "Should create scheduling constraints for multi-part workshops")
{
    auto input = R"(
(slot) s1
(slot) s2
(slot) s3

(event) e: 1-100 [3 parts]

(person) p: 1
)";

    auto data = InputReader::read_input(input);
    data.build_constraints(ConstraintParser::parse);

    CHECK(data.scheduling_constraints().size() == 6);

    for(auto const& c : data.scheduling_constraints())
    {
        if(c.type() != ConstraintType::WorkshopsAreNotInSameSlot)
        {
            REQUIRE(c.type() == ConstraintType::WorkshopsHaveOffset);
            REQUIRE(c.right() - c.left() == c.extra());
        }
    }
}

TEST_CASE(PREFIX "Should create scheduling constraints for non-optional workshops")
{
    auto input = R"(
(slot) s1

(event) e: 1-100
(event) f: 1-100 [optional]

(person) p: 1 1
)";

    auto data = InputReader::read_input(input);
    data.build_constraints(ConstraintParser::parse);

    // One WorkshopIsNotInSlot for e and one WorkshopIsInSlot for the auto-generated unassigned-slot-filling-workshop.
    REQUIRE(data.scheduling_constraints().size() == 2);
}

TEST_CASE(PREFIX "Should auto-generate slot if none is given")
{
    auto input = R"(
(event) w1: 1-100
(person) p1: 0
)";

    auto data = InputReader::read_input(input);

    REQUIRE(data.slot_count() == 1);
}

TEST_CASE(PREFIX "Should not accept too many preferences")
{
    auto input = R"(
(event) w1: 0-1
(person) p1: 0 1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept too few preferences")
{
    auto input = R"(
(event) w1: 0-1
(event) w2: 0-1
(person) p1: 0
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept workshops with zero minimum participant count")
{
    auto input = R"(
(slot) s
(event) e: 0-100
(person) p: 1 1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept input without events")
{
    auto input = R"(
(slot) s1
(person) p1:
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}

TEST_CASE(PREFIX "Should not accept input without participants")
{
    auto input = R"(
(slot) s1
(event) w1: 0-1
)";

    REQUIRE_THROWS_AS(InputReader::read_input(input), InputException);
}