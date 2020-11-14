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
     * Represents a choice in a form that is very close to the input. Final choice objects are built after all
     * input is parsed.
     */
    struct PreChoice
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
     * Parses a line containing a set description.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_set(string const& line, MutableInputData& inputData);

    /**
     * Parses a line containing a choice description.
     *
     * @param line The line to be parsed.
     * @param preChoices The list of PreChoice instances into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_choice(string const& line, vector<PreChoice>& preChoices);

    /**
     * Parses a line containing a chooser description.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_chooser(string const& line, MutableInputData& inputData);

    /**
     * Parses a line containing a chooser description in CSV format.
     *
     * @param line The line to be parsed.
     * @param inputData The mutable input data into which the result will be inserted
     * @return true if the parsing is successful.
     */
    static bool parse_line_chooser_csv(string const& line, MutableInputData& inputData);

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
     * @param preChoices The list of PreChoice instances into which the result may be inserted.
     */
    static void parse_line(string& line, MutableInputData& inputData, vector<PreChoice>& preChoices);

    /**
     * Converts all PreChoice instances into final choice descriptions.
     *
     * @param inputData The mutable input data.
     * @param preChoices The list of PreChoice instances.
     */
    static void compile_choices(MutableInputData& inputData, vector<PreChoice>& preChoices);

    /**
     * Generates extra sets if there are optional choices present (which are internally handled with a virtual
     * "not scheduled"-set and extra virtual workhops to which to assign all choosers in this virtual set).
     *
     * @param inputData The mutable input data.
     * @param preChoices The list of PreChoice instances.
     */
    static void generate_extra_sets(MutableInputData& inputData, vector<PreChoice>& preChoices);

    /**
     * Parses a string containing the input.
     *
     * @param input The input.
     * @return The resulting InputData instance.
     */
    static shared_ptr<InputData> parse(string const& input);

public:
    /**
     * Parses a string containing the input and prints status information.
     *
     * @param input The input.
     * @return The resulting InputData instance.
     */
    static shared_ptr<InputData> read_input(string const& input);
};

#include "InputReader.ipp"

