#include "common.h"

#include "../src/InputReader.h"
#include "../src/ConstraintParser.h"
#include <boost/algorithm/string.hpp>
#include <utility>
#include "../src/OutputWriter.h"
#include "../src/Status.h"

using namespace boost::algorithm;

const_ptr<InputData> parse_data(std::string const& input)
{
    auto data = InputReader::read_input(input);
    data->build_constraints(ConstraintParser::parse);

    return data;
}

std::string assignment_str(Solution const& solution)
{
    std::string solutionStr = OutputWriter::write_assignment_solution(solution);
    solutionStr = replace_all_copy(solutionStr, " ", "");
    solutionStr = replace_all_copy(solutionStr, "\"", "");
    solutionStr.erase(0, solutionStr.find('\n') + 1);
    solutionStr = trim_copy(solutionStr);
    solutionStr = replace_all_copy(solutionStr, "\n", ";");

    return solutionStr;
}


std::string scheduling_str(Solution const& solution)
{
    std::string solutionStr = OutputWriter::write_scheduling_solution(solution);
    solutionStr = replace_all_copy(solutionStr, " ", "");
    solutionStr = replace_all_copy(solutionStr, "\"", "");
    solutionStr.erase(0, solutionStr.find('\n') + 1);
    solutionStr = trim_copy(solutionStr);
    solutionStr = replace_all_copy(solutionStr, "\n", ";");

    return solutionStr;
}

std::string strip_whitespace(std::string text)
{
    text = replace_all_copy(text, " ", "");
    text = replace_all_copy(text, "\t", "");
    text = replace_all_copy(text, "\n", "");

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

const_ptr<CriticalSetAnalysis> cs(const_ptr<InputData> data, bool analyze)
{
    return std::make_shared<CriticalSetAnalysis>(std::move(data), analyze);
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
