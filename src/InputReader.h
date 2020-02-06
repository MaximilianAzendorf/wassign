#pragma once

#include "Types.h"
#include "InputData.h"

#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

class InputReader
{
private:
    InputReader() = default;

    struct PreWorkshop
    {
        string name;
        vector<string> cond;
        int min, max, parts;
        bool optional;
    };

    template<typename Parser>
    static bool parse(string const& line, Parser parser);

    static bool parse_line_slot(string const& line, MutableInputData& inputData);

    static bool parse_line_workshop(string const& line, vector<PreWorkshop>& preWorkshops);

    static bool parse_line_participant(string const& line, MutableInputData& inputData);

    static bool parse_line_participant_csv(string const& line, MutableInputData& inputData);

    static bool parse_line_constraint(string const& line, MutableInputData& inputData);

    static void parse_line(string& line, MutableInputData& inputData, vector<PreWorkshop>& preWorkshops);

    static void compile_workshops(MutableInputData& inputData, vector<PreWorkshop>& preWorkshops);

    static void generate_extra_slots(MutableInputData& inputData, vector<PreWorkshop>& preWorkshops);

    static InputData parse(string const& input);

public:
    static InputData read_input(string const& input);
};

#include "InputReader.ipp"

