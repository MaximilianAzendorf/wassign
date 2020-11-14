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

#include "Types.h"

class CriticalSet
{
private:
    ordered_set<int> _data;
    int _preference;

public:
    CriticalSet(int preference, vector<int> const& data);

    [[nodiscard]] bool is_covered_by(CriticalSet const &other) const;

    [[nodiscard]] bool is_superset_of(CriticalSet const &other) const;

    [[nodiscard]] bool contains(int item) const;

    [[nodiscard]] int size() const;

    [[nodiscard]] int preference() const;

    [[nodiscard]] ordered_set<int> const& elements() const;
};
