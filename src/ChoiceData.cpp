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

#include "ChoiceData.h"

ChoiceData::ChoiceData(string name, int min, int max)
        : _name(std::move(name)), _min(min), _max(max), _continuation(std::nullopt)
{
}

ChoiceData::ChoiceData(string name, int min, int max, int continuation)
        : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
{
}

ChoiceData::ChoiceData(string name, int min, int max, optional<int> continuation)
        : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
{
}

string const& ChoiceData::name() const
{
    return _name;
}

int ChoiceData::min() const
{
    return _min;
}

int ChoiceData::max() const
{
    return _max;
}

bool ChoiceData::has_continuation() const
{
    return _continuation.has_value();
}

int ChoiceData::continuation() const
{
    return _continuation.value();
}

optional<int> ChoiceData::opt_continuation() const
{
    return _continuation;
}
