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

#include "Scheduling.h"

#include <utility>
#include <cassert>

Scheduling::Scheduling(const_ptr<InputData> inputData)
        : _inputData(std::move(inputData))
{
}

Scheduling::Scheduling(const_ptr<InputData> inputData, vector<int> data)
        : _inputData(std::move(inputData)), _data(std::move(data))
{
    assert(_data.size() == _inputData->choice_count());
}

bool Scheduling::is_feasible() const
{
    vector<int> slotMin(_inputData->slot_count(), 0);
    vector<int> slotMax(_inputData->slot_count(), 0);

    for(int i = 0; i < _data.size(); i++)
    {
        slotMin[_data[i]] += _inputData->choice(i).min;
        slotMax[_data[i]] += _inputData->choice(i).max;
    }

    for(int i = 0; i < _inputData->slot_count(); i++)
    {
        if(slotMin[i] > _inputData->chooser_count() || slotMax[i] < _inputData->chooser_count())
        {
            return false;
        }
    }

    return true;
}

int Scheduling::slot_of(int choice) const
{
    return _data[choice];
}

InputData const& Scheduling::input_data() const
{
    return *_inputData;
}

vector<int> const& Scheduling::raw_data() const
{
    return _data;
}

int Scheduling::get_hash() const
{
    int hash = (int)(long)_inputData.get();
    for(int i : _data)
    {
        hash = hash * 97 + i;
    }

    return hash;
}

bool Scheduling::operator==(Scheduling const& other) const
{
    if(&_inputData != &other._inputData) return false;

    for(int i = 0; i < _data.size(); i++)
    {
        if(other._data[i] != _data[i])
        {
            return false;
        }
    }

    return true;
}

bool Scheduling::operator!=(Scheduling const& other) const
{
    return !(*this == other);
}
