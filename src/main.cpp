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

#include "Types.h"
#include "Util.h"
#include "Version.h"
#include "Options.h"
#include "Status.h"
#include "ShotgunSolver.h"
#include "OutputFormatter.h"
#include "input/InputReader.h"
#include "input/ConstraintBuilder.h"
#include "ShotgunSolverThreaded.h"

#include <iostream>
#include <fstream>
#include <csignal>

template<typename Stream>
string readInputStringFromStream(Stream& stream)
{
    std::stringstream res;
    string line;
    while(std::getline(stream, line))
    {
        res << line << '\n';
    }

    return res.str();
}

string readInputString(const_ptr<Options> options)
{
    if(options->input_files().empty())
    {
        return readInputStringFromStream(std::cin);
    }
    else
    {
        std::stringstream res;
        for(string const& file : options->input_files())
        {
            std::ifstream stream(file);
            if(!stream.is_open())
            {
                throw InputException("File \"" + file + "\" could not be opened.");
            }
            res << readInputStringFromStream(stream) << std::endl;
        }

        return res.str();
    }
}

void output_string(string const& text, string const& fileSuffix, const_ptr<Options> options)
{
    if(!options->output_file().empty())
    {
        std::ofstream stream(options->output_file() + fileSuffix);
        stream << text;
        stream.close();
    }
    else
    {
        std::cout << text << std::endl << std::endl;
    }
}

void signal_handler(int signal)
{
    Status::error("Abort on user request (signal " + str(signal) + ").");
    exit(signal);
}

struct sigaction old_action;
void set_signal_handler(int signal, void (*handler)(int))
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    sigaction(signal, &action, &old_action);
}

void track_progress(ShotgunSolverThreaded& solver)
{
    const auto outputInterval = milliseconds(1000);

    auto lastOutput = time_now();
    while(solver.is_running())
    {
        if(time_now() > lastOutput + outputInterval)
        {
            auto progress = solver.progress();
            string scoreStr = progress.getBestScore().is_finite() ? "Best score: " + progress.getBestScore().to_str() : "No solution yet";
            string itDepthString = progress.getLp() == 0
                    ? "; Scheduling depth: " + str(progress.schedDepth, 1)
                    : "; Progress I/A/L: " + str(progress.getIterations()) + "/" + str(progress.getAssignments()) + "/" + str(progress.getLp());

            Status::info("[Status] " + scoreStr
            + "; Time remaining: " + str(milliseconds(progress.getMillisecondsRemaining()))
            + itDepthString);
            lastOutput = time_now();
        }
        std::this_thread::sleep_for(milliseconds(5));
    }
}

int main(int argc, char** argv)
{
    set_signal_handler(SIGINT, signal_handler);
    set_signal_handler(SIGTERM, signal_handler);

    try
    {
        Rng::seed(time_now().time_since_epoch().count());

        const string header = "wassign [Version " WASSIGN_VERSION "]\n(c) 2022 Maximilian Azendorf\n";
        shared_ptr<Options> options = std::make_shared<Options>();

        auto optionsStatus = Options::parse(argc, argv, header, options);

        Status::enable_output(options);

        switch (optionsStatus)
        {
            case OPT_PARSE_OK:
                break;
            case OPT_PARSE_EXIT:
                return 0;
            case OPT_PARSE_ERROR:
            {
                Status::error("Invalid arguments.");
                return 1;
            }
        }

        string inputString = readInputString(options);

        Status::info("Processing input.");
        auto inputData = InputReader(options).read_input(inputString);

        Status::info("Read " + str(inputData->slot_count()) + " slot(s), " + str(inputData->choice_count()) + " choice(s) and " + str(inputData->chooser_count()) + " chooser(s).");

        Status::info("Found " + str(inputData->scheduling_constraints().size()) + " scheduling and "
            + str(inputData->assignment_constraints().size()) + " assignment constraints.");

        if(std::pow((double)inputData->max_preference(), options->preference_exponent()) * inputData->chooser_count() >= (double)LONG_MAX)
        {
            Status::warning("The preference exponent is too large; computations may cause an integer overflow");
        }

        Solution solution = Solution::invalid();

        auto scoring = std::make_shared<Scoring>(inputData, options);

        bool doCsAnalysis = !options->no_critical_sets() && !options->greedy() && inputData->slot_count() > 1;
        Status::info(doCsAnalysis ? "Performing critical set analysis." : "Skipping critical set analysis.");
        auto csAnalysis = std::make_shared<CriticalSetAnalysis>(inputData, doCsAnalysis);

        if(doCsAnalysis)
        {
            Status::info("Critical set analysis gives a preference bound of " + str(csAnalysis->preference_bound()) + ".");
        }

        Status::info("Generating static data and starting solver.");
        auto staticData = std::make_shared<MipFlowStaticData>(inputData);
        ShotgunSolverThreaded solver(inputData, csAnalysis, staticData, scoring, options);
        solver.start();

        track_progress(solver);

        Status::info("Solver finished, waiting for result.");
        solution = solver.wait_for_result();

        if (solution.is_invalid())
        {
            Status::info_important("No solution found.");
        }
        else
        {
            Status::info_important("Solution found.");
            Status::info("Solution score: " + scoring->evaluate(solution).to_str());
            if (inputData->slot_count() > 1)
            {
                string schedulingSolutionString = OutputFormatter::write_scheduling_solution(solution);
                output_string(schedulingSolutionString, ".scheduling.csv", options);
            }

            string assignmentSolutionString = OutputFormatter::write_assignment_solution(solution);
            output_string(assignmentSolutionString, ".assignment.csv", options);
        }
    }
    catch(InputException const& ex)
    {
        Status::error("Error in input: " + ex.message());
        return 1;
    }

    return 0;
}
