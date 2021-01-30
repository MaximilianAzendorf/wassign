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

#include "OutputWriter.h"

#include <sstream>

string OutputWriter::write_scheduling_solution(Solution const& solution)
{
    using std::endl;
    std::stringstream str;

    str << R"("Choice", "Set")" << endl;

    for(int w = 0; w < solution.input_data().choice_count(); w++)
    {
        int s = solution.scheduling()->set_of(w);
        string wName = solution.input_data().choice(w).name;
        if(wName.rfind(InputData::HiddenChoicePrefix, 0) == 0)
        {
            continue;
        }

        string sName = solution.input_data().set(s).name;
        if(sName.rfind(InputData::NotScheduledSetPrefix, 0) == 0)
        {
            sName = "not scheduled";
        }

        if(wName.rfind(InputData::GeneratedPrefix, 0) == 0)
        {
            wName = wName.substr(InputData::GeneratedPrefix.length());
        }

        str << '"' << wName << '"' << ", " << '"' << sName << '"' << endl;
    }

    return str.str();
}

string OutputWriter::write_assignment_solution(Solution const& solution)
{
    using std::endl;
    std::stringstream str;

    str << "\"Choice\"";

    for(int s = 0; s < solution.input_data().set_count(); s++)
    {
        if(solution.input_data().set(s).name.rfind(InputData::NotScheduledSetPrefix, 0) == 0)
        {
            continue;
        }

        str << ", \"" << solution.input_data().set(s).name << '"';
    }

    for(int p = 0; p < solution.input_data().chooser_count(); p++)
    {
vector<int> choices(solution.input_data().set_count());
        for(int s = 0; s < solution.input_data().set_count(); s++)
        {
            int ws = solution.assignment()->choice_of(p, s);
            choices[solution.scheduling()->set_of(ws)] = ws;
        }

        str << endl << '"' << solution.input_data().chooser(p).name << '"';

        for(int s = 0; s < solution.input_data().set_count(); s++)
        {
            if(solution.input_data().set(s).name.rfind(InputData::NotScheduledSetPrefix, 0) == 0)
            {
                continue;
            }

            string wName = solution.input_data().choice(choices[s]).name;
            if(wName.rfind(InputData::GeneratedPrefix, 0) == 0)
            {
                wName = wName.substr(InputData::GeneratedPrefix.length());
            }
            str << ", \"" << wName << "\"";
        }
    }

    return str.str();
}
