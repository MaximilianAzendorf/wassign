#include "Types.h"
#include "Util.h"
#include "Version.h"
#include "Options.h"
#include "Status.h"
#include "InputReader.h"
#include "MipSolver.h"
#include "OutputWriter.h"
#include "ConstraintParser.h"

#include <iostream>
#include <fstream>

template<typename Stream>
string readInputString(Stream& stream)
{
    std::stringstream res;
    string line;
    while(std::getline(stream, line))
    {
        res << line << '\n';
    }

    return res.str();
}

string readInputString()
{
    if(Options::input_files().empty())
    {
        return readInputString(std::cin);
    }
    else
    {
        std::stringstream res;
        for(string const& file : Options::input_files())
        {
            std::ifstream stream(file);
            if(!stream.is_open())
            {
                throw InputException("File \"" + file + "\" could not be opened.");
            }
            res << readInputString(stream) << std::endl;
        }

        return res.str();
    }
}

void output_string(string const& text, string const& fileSuffix)
{
    if(!Options::output_file().empty())
    {
        std::ofstream stream(Options::output_file() + fileSuffix);
        stream << text;
        stream.close();
    }
    else
    {
        std::cout << text << std::endl << std::endl;
    }
}

int main(int argc, char** argv)
{
    try
    {
        Rng::seed(time_now().time_since_epoch().count());

        const string header = "wsolve [Version " WSOLVE_VERSION "]\n(c) 2019 Maximilian Azendorf\n";
        switch (Options::parse(argc, argv, header))
        {
            case OK:
                break;
            case EXIT:
                return 0;
            case ERROR:
            {
                Status::error("Invalid arguments.");
                break;
            }
        }

        string inputString = readInputString();
        InputData inputData = InputReader::read_input(inputString);
        inputData.build_constraints(ConstraintParser::parse);

        Status::info("Found " + str(inputData.scheduling_constraints().size()) + " scheduling and "
            + str(inputData.assignment_constraints().size()) + " assignment constraints.");

        MipSolver solver;
        Solution solution = solver.solve(inputData);

        if (solution.is_invalid())
        {
            Status::warning("No solution found.");
        }
        else
        {
            Status::info("Solution score: " + Scoring(inputData).evaluate(solution).to_str());
            if (inputData.slot_count() > 1)
            {
                string schedulingSolutionString = OutputWriter::write_scheduling_solution(solution);
                output_string(schedulingSolutionString, ".scheduling.csv");
            }

            string assignmentSolutionString = OutputWriter::write_assignment_solution(solution);
            output_string(assignmentSolutionString, ".assignment.csv");
        }
    }
    catch(InputException const& ex)
    {
        Status::error("Error in input: " + ex.message());
        return 1;
    }

    return 0;
}