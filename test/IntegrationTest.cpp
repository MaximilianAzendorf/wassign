#include "common.h"
#include "../src/Solution.h"
#include "../src/InputReader.h"
#include "../src/ConstraintParser.h"
#include "../src/MipSolver.h"
#include "../src/Options.h"
#include "../src/OutputWriter.h"
#include "../src/Status.h"

#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

#define PREFIX "[Integration] "

Solution solve(std::string input, int timeout = 1)
{
    auto data = new InputData(InputReader::read_input(input));
    data->build_constraints(ConstraintParser::parse);

    auto options = Options::default_options();
    options.set_thread_count(1);
    options.set_timeout_seconds(timeout);
    options.set_verbosity(3);

    Status::enable_output(options);

    MipSolver solver(options);
    Solution solution = solver.solve(*data);

    return solution;
}

void expect_assignment(Solution solution, std::string expectation)
{
    std::string solutionStr = OutputWriter::write_assignment_solution(solution);
    solutionStr = replace_all_copy(solutionStr, " ", "");
    solutionStr = replace_all_copy(solutionStr, "\"", "");
    solutionStr.erase(0, solutionStr.find("\n") + 1);
    solutionStr = trim_copy(solutionStr);
    solutionStr = replace_all_copy(solutionStr, "\n", ";");

    expectation = replace_all_copy(expectation, " ", "");
    REQUIRE(solutionStr == expectation);
}

void expect_scheduling(Solution solution, std::string expectation)
{
    std::string solutionStr = OutputWriter::write_scheduling_solution(solution);
    solutionStr = replace_all_copy(solutionStr, " ", "");
    solutionStr = replace_all_copy(solutionStr, "\"", "");
    solutionStr.erase(0, solutionStr.find("\n") + 1);
    solutionStr = trim_copy(solutionStr);
    solutionStr = replace_all_copy(solutionStr, "\n", ";");

    expectation = replace_all_copy(expectation, " ", "");
    REQUIRE(solutionStr == expectation);
}

TEST_CASE(PREFIX "Minimal")
{
    auto input = R"(
(slot) s
(event) e: 1-1
(person) p: 1
)";

    auto solution = solve(input);
    expect_assignment(solution, "p,e");
    expect_scheduling(solution, "e,s");
}

TEST_CASE(PREFIX "Single slot")
{
    auto input = R"(
(slot) s
(event) e1: 3-3
(event) e2: 3-3
(person) p1: 1 0
(person) p2: 0 1
(person) p3: 0 1
(person) p4: 1 0
(person) p5: 1 0
(person) p6: 1 1
)";

    auto solution = solve(input);
    expect_assignment(solution, "p1,e1; p2,e2; p3,e2; p4,e1; p5,e1; p6,e2");
    expect_scheduling(solution, "e1,s; e2,s");
}

TEST_CASE(PREFIX "Single slot with assignment constraint")
{
    auto input = R"(
(slot) s
(event) e1: 3-3
(event) e2: 3-3
(person) p1: 2 0
(person) p2: 0 2
(person) p3: 0 2
(person) p4: 2 0
(person) p5: 2 1
(person) p6: 2 2
(constraint) participants of [e2] contain not [p6]
)";

    auto solution = solve(input);
    expect_assignment(solution, "p1,e1; p2,e2; p3,e2; p4,e1; p5,e2; p6,e1");
    expect_scheduling(solution, "e1,s; e2,s");
}

TEST_CASE(PREFIX "Multiple slots with scheduling constraint")
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

    auto solution = solve(input);
    expect_assignment(solution, "p1,e1,e2; p2,e1,e3; p3,e1,e3");
    expect_scheduling(solution, "e1,s1; e2,s2; e3,s2");
}