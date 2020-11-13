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