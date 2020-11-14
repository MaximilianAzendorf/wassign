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

#include "ParticipantData.h"

ParticipantData::ParticipantData(string name, vector<int> preferences)
        : _name(std::move(name)), _preferences(std::move(preferences))
{
}

string const& ParticipantData::name() const
{
    return _name;
}

int ParticipantData::preference(int workshopIndex) const
{
    return _preferences[workshopIndex];
}

vector<int> const& ParticipantData::preferences() const
{
    return _preferences;
}
