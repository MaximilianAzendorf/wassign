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

#include "CriticalSet.h"

#include <algorithm>

CriticalSet::CriticalSet(int preference, vector<int> const& data)
        : _preference(preference)
{
    _data.insert(data.begin(), data.end());
}

bool CriticalSet::is_covered_by(CriticalSet const& other) const
{
    return _preference <= other._preference && is_superset_of(other);
}

bool CriticalSet::is_superset_of(CriticalSet const& other) const
{
    return std::includes(_data.begin(), _data.end(), other._data.begin(), other._data.end());
}

bool CriticalSet::contains(int item) const
{
    return _data.find(item) != _data.end();
}

int CriticalSet::size() const
{
    return _data.size();
}

int CriticalSet::preference() const
{
    return _preference;
}

ordered_set<int> const& CriticalSet::elements() const
{
    return _data;
}
