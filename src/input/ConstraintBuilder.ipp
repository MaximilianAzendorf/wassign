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

#include "ConstraintBuilder.h"
#include "InputException.h"

template<typename Data>
int ConstraintBuilder::find_name(string const& name, vector<Data> const& dataVector)
{
    vector<string> names;
    for(int i = 0; i < dataVector.size(); i++)
    {
        names.push_back(dataVector[i].name);
    }

    auto res = FuzzyMatch::find(name, names);

    if(res.size() > 1)
    {
        throw InputException("The name \"" + name + "\" is ambiguous.");
    }
    else if(res.size() == 0)
    {
        return -1;
    }
    else
    {
        return res.front();
    }
}