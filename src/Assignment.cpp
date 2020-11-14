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

#include "Assignment.h"

#include <climits>
#include <cassert>
#include <utility>

Assignment::Assignment(const_ptr<InputData> inputData, vector<vector<int>> data)
        : _inputData(std::move(inputData)), _data(std::move(data))
{
    assert(_data.size() == _inputData->chooser_count());

    for(vector<int> const& i : _data)
    {
        assert(i.size() == _inputData->set_count());
    }
}

int Assignment::choice_of(int chooser, int set) const
{
    return _data[chooser][set];
}

vector<int> Assignment::choosers_ordered(int choice) const
{
    vector<int> choosers;
    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            if(choice_of(p, s) == choice)
            {
                choosers.push_back(p);
            }
        }
    }

    std::sort(choosers.begin(), choosers.end());
    return choosers;
}

vector<int> Assignment::choices_ordered(int chooser) const
{
    vector<int> choices;
    for(int s = 0; s < _inputData->set_count(); s++)
    {
        choices.push_back(choice_of(chooser, s));
    }

    std::sort(choices.begin(), choices.end());
    return choices;
}

bool Assignment::is_in_choice(int chooser, int choice) const
{
    for(int s = 0; s < _inputData->set_count(); s++)
    {
        if(choice_of(chooser, s) == choice) return true;
    }

    return false;
}

int Assignment::max_used_preference() const
{
    int c = INT_MIN;
    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            c = std::max(_inputData->chooser(p).preference(choice_of(p, s)), c);
        }
    }

    return c;
}

InputData const& Assignment::input_data() const
{
    return *_inputData;
}

bool Assignment::operator==(Assignment const& other) const
{
    if(&_inputData != &other._inputData) return false;

    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            if(choice_of(p, s) != other.choice_of(p, s))
            {
                return false;
            }
        }
    }

    return true;
}

bool Assignment::operator!=(Assignment const& other) const
{
    return !(*this == other);
}
