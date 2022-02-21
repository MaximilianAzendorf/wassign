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

/**
 * Represents a single scheduling solution for the given input data. A scheduling is a mapping between choices
 * and slots.
 */
class Scheduling
{
private:
    const_ptr<InputData> _inputData;
    vector<int> _data;

public:
    inline static const int NOT_SCHEDULED = -1;

    /**
     * Constructor.
     */
    explicit Scheduling(const_ptr<InputData> inputData);

    /**
     * Constructor
     *
     * @param data A vector v of integers where v[w] is the slot of w.
     */
    Scheduling(const_ptr<InputData> inputData, vector<int> data);

    /**
     * Returns true if a solution is possible with this scheduling.
     */
    [[nodiscard]] bool is_feasible() const;

    /**
     * Returns the slot of the given choice in this scheduling.
     */
    [[nodiscard]] int slot_of(int choice) const;

    /**
     * Returns the input data this scheduling is based on.
     */
    [[nodiscard]] InputData const& input_data() const;

    /**
     * Returns the raw data vector of this scheduling.
     */
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


