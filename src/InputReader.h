#pragma once

#include "Types.h"
#include "InputData.h"

#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

class InputReader
{
private:
    InputReader() = default;

    /**
     * Represents a workshop in a form that is very close to the input. Final workshop objects are built after all
     * input is parsed.
     */
    struct PreWorkshop
    {
        string name;
        vector<string> cond;
        int min, max, parts;
        bool optional;
    };

    /**
     * Passes a single input line through a boost spirit parser.
     *
     * @tparam Parser A boost spirit parser.
     * @param line The line to be parsed.
     * @param parser A boost spirit parser.
     * @return true if the parsing is successful.
     */
    template<typename Parser>
    static bool parse(string const& line, Parser parser);

    /**
     * Parses a line containing a slot description.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_slot(string const& line, MutableInputData& inputData);

    /**
     * Parses a line containing a workshop description.
     *
     * @param line The line to be parsed.
     * @param preWorkshops The list of PreWorkshop instances into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_workshop(string const& line, vector<PreWorkshop>& preWorkshops);

    /**
     * Parses a line containing a participant description.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_participant(string const& line, MutableInputData& inputData);

    /**
     * Parses a line containing a participant description in CSV format.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_participant_csv(string const& line, MutableInputData& inputData);

    /**
     * Parses a line containing a constraint description.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_constraint(string const& line, MutableInputData& inputData);

    /**
     * Parses a single input line.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result may be inserted.
     * @param preWorkshops The list of PreWorkshop instances into which the result may be inserted.
     */
    static void parse_line(string& line, MutableInputData& inputData, vector<PreWorkshop>& preWorkshops);

    /**
     * Converts all PreWorkshop instances into final workshop descriptions.
     *
     * @param inputData The mutable input data.
     * @param preWorkshops The list of PreWorkshop instances.
     */
    static void compile_workshops(MutableInputData& inputData, vector<PreWorkshop>& preWorkshops);

    /**
     * Generates extra slots if there are optional workshops present (which are internally handled with a virtual
     * "not scheduled"-slot and extra virtual workhops to which to assign all participants in this virtual slot).
     *
     * @param inputData The mutable input data.
     * @param preWorkshops The list of PreWorkshop instances.
     */
    static void generate_extra_slots(MutableInputData& inputData, vector<PreWorkshop>& preWorkshops);

    /**
     * Parses a string containing the input.
     *
     * @param input The input.
     * @return The resulting InputData instance.
     */
    static InputData parse(string const& input);

public:
    /**
     * Parses a string containing the input and prints status information.
     *
     * @param input The input.
     * @return The resulting InputData instance.
     */
    static InputData read_input(string const& input);
};

#include "InputReader.ipp"

