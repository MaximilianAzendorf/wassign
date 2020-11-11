/**
 * This file defines the public API available through embind-generated WASM bindings.
 */

#ifdef __EMCC__

#include <emscripten/bind.h>

#include <utility>

#include "Types.h"
#include "ShotgunSolver.h"
#include "InputReader.h"
#include "Status.h"
#include "ConstraintParser.h"
#include "OutputWriter.h"

using namespace emscripten;

class JsSolution
{
private:
    string _scheduling_solution;
    string _assignment_solution;

public:
    JsSolution(string schedSolution, string assignmentSolution)
        : _scheduling_solution(std::move(schedSolution)),
          _assignment_solution(std::move(assignmentSolution))
    {
    }

    string get_scheduling_solution() const { return _scheduling_solution; }
    string get_assignment_solution() const { return _assignment_solution; }
};

JsSolution jsSolve(const string& inputString, const Options& options)
{
    Rng::seed(time_now().time_since_epoch().count());

    InputData inputData = InputReader::read_input(inputString);
    inputData.build_constraints(ConstraintParser::parse);

    ShotgunSolver solver(options);
    Solution solution = solver.solve(inputData);

    if (solution.is_invalid())
    {
        Status::info_important("No solution found.");

        return JsSolution("", "");
    }
    else
    {
        Status::info_important("Solution found.");
        Status::info("Solution score: " + Scoring(inputData, options).evaluate(solution).to_str());

        string schedulingSolutionString = inputData.slot_count() > 1 ? OutputWriter::write_scheduling_solution(solution) : "";
        string assignmentSolutionString = OutputWriter::write_assignment_solution(solution);

        return JsSolution(schedulingSolutionString, assignmentSolutionString);
    }
}

JsSolution jsSolveEasy(const string& inputString)
{
    auto options = Options::default_options();
    options.set_verbosity(4);
    options.set_timeout_seconds(10);
    return jsSolve(inputString, options);
}

EMSCRIPTEN_BINDINGS(wsolve)
{
    function("solve", &jsSolve);
    function("solve_easy", &jsSolveEasy);

    class_<JsSolution>("Solution")
        .constructor<string, string>()
        .property("assignmentSolution", &JsSolution::get_assignment_solution)
        .property("schedulingSolution", &JsSolution::get_scheduling_solution);

    class_<Options>("Options")
        .constructor()
        .property("verbosity", &Options::verbosity, &Options::set_verbosity)
        .property("timeout", &Options::timeout_seconds, &Options::set_timeout_seconds)
        .property("csTimeout", &Options::critical_set_timeout_seconds, &Options::set_critical_set_timeout_seconds)
        .property("noCriticalSets", &Options::no_critical_sets, &Options::set_no_critical_sets)
        .property("preferenceExponent", &Options::preference_exponent, &Options::set_preference_exponent)
        .property("any", &Options::any, &Options::set_any)
        .property("threadCount", &Options::thread_count, &Options::set_thread_count);
}

#endif