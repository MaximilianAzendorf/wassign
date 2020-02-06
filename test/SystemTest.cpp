#include "common.h"

#include "../src/Solution.h"
#include "../src/InputReader.h"
#include "../src/MipSolver.h"
#include "../src/ConstraintParser.h"

Solution solve(string input)
{
    auto data = InputReader::read_input(input);
    data.build_constraints(ConstraintParser::parse);

    MipSolver solver{};
    return solver.solve(data);
}

TEST_CASE("Test inputs", INTEGRATION)
{
    SECTION("Minimal possible inputs")
    {
        auto s = solve(R"(
(slot) s
(event) w: 1-1
(person) p: 1
)");

        REQUIRE(s.scheduling().slot_of(0) == 0);
        REQUIRE(s.assignment().workshop_of(0, 0) == 0);
    }
}
