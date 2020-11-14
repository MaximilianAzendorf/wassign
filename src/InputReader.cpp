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

#include "InputReader.h"

#include "Status.h"
#include "Util.h"
#include "SpiritUtil.h"

#include <boost/algorithm/string.hpp>

using x3::char_;
using x3::uint_;
using x3::lit;
using x3::lexeme;

bool InputReader::parse_line_set(string const& line, MutableInputData& inputData)
{
    string name;

    auto syntax = lit("(set)") >> lexeme[ (*char_)[pset(name)] ];

    if(!parse(line, syntax)) return false;
    inputData.sets.push_back(SetData(name));
    return true;
}

bool InputReader::parse_line_choice(string const& line, vector<InputReader::PreChoice>& preChoices)
{
    PreChoice ws{};

    auto name = *(char_ - char_(":,[]"));
    auto syntax =
            lit("(choice)")
                    >> lexeme[ name[pset(ws.name)] ] >> ':'
                    >> uint_[pset(ws.min)] >> '-' >> uint_[pset(ws.max)]
                    >> -('['
                            >> ((uint_[pset(ws.parts)] >> "parts")
                                | lit("optional")[pset(ws.optional, true)]
                                | lit("fixed") >> name[padd(ws.cond)]
                                ) % ','
                            >> ']');


    if(!parse(line, syntax)) return false;

    if(ws.optional && ws.parts > 1)
    {
        // TODO: Implement support for this
        throw InputException("Optional choices with multiple parts are not supported.");
    }

    if(!ws.optional && ws.min < 1)
    {
        throw InputException("Choices with no minimum number of choosers are not supported. Make them optional instead.");
    }

    boost::algorithm::trim(ws.name);

    preChoices.push_back(ws);
    return true;
}


bool InputReader::parse_line_chooser(string const& line, MutableInputData& inputData)
{
    string name;
    vector<int> pref;

    auto syntax =
            lit("(chooser)")
                    >> lexeme[ (*(char_ - ':'))[pset(name)] ]
                    >> ':'
                    >> +(uint_[padd(pref)]);

    if(!parse(line, syntax)) return false;

    for(int i = 0; i < pref.size(); i++)
    {
        pref[i] = -pref[i];
    }

    boost::algorithm::trim(name);

    inputData.choosers.push_back(ChooserData(name, pref));
    return true;
}

bool InputReader::parse_line_chooser_csv(string const& line, MutableInputData& inputData)
{
    string name;
    vector<int> pref;

    auto syntax =
            (*(char_ - ','))[pset(name)]
            >> ','
            >> uint_[padd(pref)] % ',';

    if(!parse(line, syntax)) return false;

    boost::algorithm::trim(name);

    inputData.choosers.push_back(ChooserData(name, pref));
    return true;

}

bool InputReader::parse_line_constraint(string const& line, MutableInputData& inputData)
{
    string constraint;

    auto syntax = lit("(constraint)") >> lexeme[ (*char_)[pset(constraint)] ];

    if(!parse(line, syntax)) return false;
    inputData.constraintStrings.push_back(constraint);
    return true;
}

void InputReader::parse_line(string& line, MutableInputData& inputData, vector<InputReader::PreChoice>& preChoices)
{
    boost::algorithm::trim(line);
    if(line.empty() || line.front() == '#')
    {
        return;
    }

    if(!parse_line_set(line, inputData)
       && !parse_line_choice(line, preChoices)
       && !parse_line_chooser(line, inputData)
       && !parse_line_constraint(line, inputData)
       && !parse_line_chooser_csv(line, inputData))
    {
        throw InputException("Could not parse line \"" + line + "\".");
    }
}

void InputReader::compile_choices(MutableInputData& inputData, vector<InputReader::PreChoice>& preChoices)
{
    int wsidx = 0;

    for(PreChoice const& pw : preChoices)
    {
        vector<int> conductors;
        for(string const& name : pw.cond)
        {
            bool found = false;
            for(int p = 0; p < inputData.choosers.size(); p++)
            {
                if(inputData.choosers[p].name() == name)
                {
                    conductors.push_back(p);
                    vector<int> newPref(inputData.choosers[p].preferences());
                    newPref[wsidx] = InputData::MinPrefPlaceholder;
                    inputData.choosers[p] = ChooserData(inputData.choosers[p].name(), newPref);
                    inputData.conductors.push_back({.chooser = p, .choice = wsidx});

                    found = true;
                    break;
                }
            }

            if(!found) throw InputException("Unknown chooser \"" + name + "\".");
        }

        inputData.choices.push_back(ChoiceData(
                pw.name,
                pw.min,
                pw.max,
                pw.parts > 1 ? std::make_optional(wsidx + 1) : std::nullopt));

        wsidx++;
        if(pw.parts > 1)
        {
            for(int p = 0; p < inputData.choosers.size(); p++)
            {
                vector<int> newPrefs;
                for(int i = 0; i < inputData.choosers[p].preferences().size(); i++)
                {
                    for(int j = 0; j < (i == wsidx - 1 ? pw.parts : 1); j++)
                    {
                        newPrefs.push_back(inputData.choosers[p].preference(i));
                    }
                }

                inputData.choosers[p] = ChooserData(inputData.choosers[p].name(), newPrefs);
            }

            for(int i = 1; i < pw.parts; i++)
            {
                string name = InputData::GeneratedPrefix + "[" + str(i + 1) + "] " + pw.name;
                inputData.choices.push_back(ChoiceData(
                        name,
                        pw.min,
                        pw.max,
                        i == pw.parts - 1 ? std::nullopt : std::make_optional(wsidx + 1)));
                wsidx++;
            }
        }
    }
}

void InputReader::generate_extra_sets(MutableInputData& inputData, vector<InputReader::PreChoice>& preChoices)
{
    int optMin = 0;
    for(PreChoice pw : preChoices)
    {
        if(!pw.optional) continue;
        optMin += pw.min;
    }

    int numExtraSets = (int)std::ceil((double)optMin / (double)inputData.choosers.size());

    for(int i = 0; i < numExtraSets; i++)
    {
        string extraSet = InputData::NotScheduledSetPrefix + str(i);
        string extraChoice = InputData::HiddenChoicePrefix + "unassigned_" + str(i);

        int s = inputData.sets.size();

        inputData.sets.push_back(SetData(extraSet));
        inputData.choices.push_back(ChoiceData(extraChoice, 0, inputData.choosers.size() + 1));
        inputData.constraints.push_back(Constraint(ChoiceIsInSet, inputData.choices.size() - 1, s));

        for(int p = 0; p < inputData.choosers.size(); p++)
        {
            vector<int> newPref(inputData.choosers[p].preferences());
            newPref.push_back(InputData::MinPrefPlaceholder);
            inputData.choosers[p] = ChooserData(inputData.choosers[p].name(), newPref);
        }

        for(PreChoice pw : preChoices)
        {
            if(pw.optional) continue;

            int w = 0;
            for(; w < inputData.choices.size(); w++)
            {
                if(inputData.choices[w].name() == pw.name) break;
            }

            inputData.constraints.push_back(Constraint(ChoiceIsNotInSet, w, s));
        }
    }
}

shared_ptr<InputData> InputReader::parse(string const& input)
{
    std::stringstream inputStream(input);
    string line;

    MutableInputData inputData;
    vector<PreChoice> preChoices;

    while(std::getline(inputStream, line, '\n'))
    {
        if(line.back() == '\r') line.resize(line.size() - 1);
        parse_line(line, inputData, preChoices);
    }

    compile_choices(inputData, preChoices);
    generate_extra_sets(inputData, preChoices);

    return std::make_shared<InputData>(inputData);
}

shared_ptr<InputData> InputReader::read_input(string const& input)
{
    Status::info("Begin parsing input.");

    auto res = parse(input);
    int setCount = res->set_count();
    int wsCount = res->choice_count();
    int partCount = res->chooser_count();

    if(partCount == 0)
    {
        throw InputException("No chooser(s) given in input.");
    }

    for(int i = 0; i < res->set_count(); i++)
    {
        if(res->set(i).name().rfind(InputData::GeneratedPrefix, 0) == 0)
        {
            setCount--;
        }
    }

    for(int i = 0; i < res->choice_count(); i++)
    {
        if(res->choice(i).name().rfind(InputData::GeneratedPrefix, 0) == 0)
        {
            wsCount--;
        }
    }

    for(int i = 0; i < res->chooser_count(); i++)
    {
        if(res->chooser(i).name().rfind(InputData::GeneratedPrefix, 0) == 0)
        {
            partCount--;
        }
    }

    Status::info("Read "
                 + str(setCount) + " set(s), "
                 + str(wsCount) + " choice(s) and "
                 + str(partCount) + " chooser(s).");

    return res;
}