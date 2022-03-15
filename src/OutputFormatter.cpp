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

#include "OutputFormatter.h"

#include <sstream>

string OutputFormatter::write_scheduling_solution(Solution const& solution)
{
    using std::endl;
    std::stringstream str;

    str << R"("Choice", "Slot")" << endl;

    for(int w = 0; w < solution.input_data().choice_count(); w++)
    {
        int s = solution.scheduling()->slot_of(w);
        string wName = solution.input_data().choice(w).name;
        string sName = s == Scheduling::NOT_SCHEDULED ? "not scheduled" : solution.input_data().slot(s).name;

        if(wName.rfind(InputData::GeneratedPrefix, 0) == 0)
        {
            wName = wName.substr(InputData::GeneratedPrefix.length());
        }

        str << '"' << wName << '"' << ", " << '"' << sName << '"' << endl;
    }

    return str.str();
}

string OutputFormatter::write_assignment_solution(Solution const& solution)
{
    using std::endl;
    std::stringstream str;

    str << "\"Chooser\"";

    for(int s = 0; s < solution.input_data().slot_count(); s++)
    {
        str << ", \"" << solution.input_data().slot(s).name << '"';
    }

    for(int p = 0; p < solution.input_data().chooser_count(); p++)
    {
vector<int> choices(solution.input_data().slot_count());
        for(int s = 0; s < solution.input_data().slot_count(); s++)
        {
            int ws = solution.assignment()->choice_of(p, s);
            choices[solution.scheduling()->slot_of(ws)] = ws;
        }

        str << endl << '"' << solution.input_data().chooser(p).name << '"';

        for(int s = 0; s < solution.input_data().slot_count(); s++)
        {
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
