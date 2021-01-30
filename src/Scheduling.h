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
#include "InputData.h"

class Scheduling
{
private:
    const_ptr<InputData> _inputData;
    vector<int> _data;

public:
    explicit Scheduling(const_ptr<InputData> inputData);

    Scheduling(const_ptr<InputData> inputData, vector<int> data);

    [[nodiscard]] bool is_feasible() const;

    [[nodiscard]] int set_of(int choice) const;

    [[nodiscard]] InputData const& input_data() const;

    [[nodiscard]] vector<int> const& raw_data() const;

    [[nodiscard]] int get_hash() const;

    bool operator == (Scheduling const& other) const;

    bool operator != (Scheduling const& other) const;
};

namespace std
{
    template <>
    struct hash<Scheduling>
    {
        size_t operator()(Scheduling const& scheduling) const
        {
            return scheduling.get_hash();
        }
    };
}


