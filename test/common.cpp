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
#include "../src/input/ConstraintBuilder.h"
#include <utility>
#include <regex>
#include "../src/OutputFormatter.h"
#include "../src/Status.h"
#include "../src/ShotgunSolverThreaded.h"

const_ptr<InputData> parse_data(std::string const& input)
{
    auto data = InputReader(Options::default_options()).read_input(input);

    return data;
}

std::string trim_copy(std::string const& str)
{
    std::string res = std::string(str);
    res.erase(res.begin(), std::find_if(res.begin(), res.end(), [](char ch){ return !std::isspace(ch);}));
    res.erase(std::find_if(res.rbegin(), res.rend(), [](char ch){ return !std::isspace(ch);}).base(), res.end());
    return res;
}

std::string assignment_str(Solution const& solution)
{
    std::string solutionStr = OutputFormatter::write_assignment_solution(solution);
    solutionStr = std::regex_replace(solutionStr, std::regex(" "), "");
    solutionStr = std::regex_replace(solutionStr, std::regex("\""), "");
    solutionStr.erase(0, solutionStr.find('\n') + 1);
    solutionStr = trim_copy(solutionStr);
    solutionStr = std::regex_replace(solutionStr, std::regex("\\n"), ";");

    return solutionStr;
}


std::string scheduling_str(Solution const& solution)
{
    std::string solutionStr = OutputFormatter::write_scheduling_solution(solution);
    solutionStr = std::regex_replace(solutionStr, std::regex(" "), "");
    solutionStr = std::regex_replace(solutionStr, std::regex("\""), "");
    solutionStr.erase(0, solutionStr.find('\n') + 1);
    solutionStr = trim_copy(solutionStr);
    solutionStr = std::regex_replace(solutionStr, std::regex("\\n"), ";");

    return solutionStr;
}

std::string strip_whitespace(std::string text)
{
    text = std::regex_replace(text, std::regex(" "), "");
    text = std::regex_replace(text, std::regex("\\t"), "");
    text = std::regex_replace(text, std::regex("\\n"), "");

    return text;
}

void expect_assignment(Solution const& solution, std::string expectation)
{
    std::string solutionStr = assignment_str(solution);

    expectation = strip_whitespace(expectation);
    REQUIRE(solutionStr == expectation);
}

void expect_scheduling(Solution const& solution, std::string expectation)
{
    std::string solutionStr = scheduling_str(solution);

    expectation = strip_whitespace(expectation);
    REQUIRE(solutionStr == expectation);
}

const_ptr<CriticalSetAnalysis> csa(const_ptr<InputData> data, bool analzye)
{
    return std::make_shared<CriticalSetAnalysis>(std::move(data), analzye);
}

const_ptr<MipFlowStaticData> sd(const_ptr<InputData> data)
{
    return std::make_shared<MipFlowStaticData>(std::move(data));
}

shared_ptr<Options> default_options()
{
    auto options = Options::default_options();

    options->set_thread_count(13);
    options->set_timeout_seconds(1);
    options->set_verbosity(3);

    Status::enable_output(options);

    return options;
}

const_ptr<Scoring> scoring(const_ptr<InputData> inputData, const_ptr<Options> options)
{
    return std::make_shared<Scoring>(std::move(inputData), std::move(options));
}

Solution sol(const_ptr<Scheduling> scheduling)
{
    return Solution(std::move(scheduling), nullptr);
}

Solution sol(const_ptr<Assignment> assignment)
{
    return Solution(nullptr, std::move(assignment));
}

Solution sol(const_ptr<Scheduling> scheduling, const_ptr<Assignment> assignment)
{
    return Solution(std::move(scheduling), std::move(assignment));
}

Solution solve(const_ptr<InputData> data, int timeout)
{
    auto options = default_options();
    options->set_timeout_seconds(timeout);

    ShotgunSolverThreaded solver(data, csa(data), sd(data), scoring(data, options), options);
    solver.start();

    Solution sol = solver.wait_for_result();

    Status::info("Iterations calculated: " + str(solver.progress().iterations));

    return sol;
}