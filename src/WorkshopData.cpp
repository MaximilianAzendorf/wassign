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

#include "WorkshopData.h"

WorkshopData::WorkshopData(string name, int min, int max)
        : _name(std::move(name)), _min(min), _max(max), _continuation(std::nullopt)
{
}

WorkshopData::WorkshopData(string name, int min, int max, int continuation)
        : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
{
}

WorkshopData::WorkshopData(string name, int min, int max, optional<int> continuation)
        : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
{
}

string const& WorkshopData::name() const
{
    return _name;
}

int WorkshopData::min() const
{
    return _min;
}

int WorkshopData::max() const
{
    return _max;
}

bool WorkshopData::has_continuation() const
{
    return _continuation.has_value();
}

int WorkshopData::continuation() const
{
    return _continuation.value();
}

optional<int> WorkshopData::opt_continuation() const
{
    return _continuation;
}
