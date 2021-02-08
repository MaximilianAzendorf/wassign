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
 * Represents a single assignment solution for the given input data. An assignment is a mapping between choosers
 * and choices.
 */
class Assignment
{
private:
    const_ptr<InputData> _inputData;
    vector<vector<int>> _data;

public:
    /**
     * Constructor.
     *
     * @param inputData The input data this instance is an assignment solution for.
     * @param data A vector of vectors so that chooser x is assigned to choice data[x][y] at slot y.
     */
    Assignment(const_ptr<InputData> inputData, vector<vector<int>> data);

    /**
     * Returns the choice the given chooser is assigned to at the given slot.
     */
    [[nodiscard]] int choice_of(int chooser, int slot) const;

    /**
     *
     * Returns an ordered vector of all choosers that are assigned to the given choice.
     */
    [[nodiscard]] vector<int> choosers_ordered(int choice) const;

    /**
     * Returns an ordered vector of all choices to which the given chooser is assigned.
     */
    [[nodiscard]] vector<int> choices_ordered(int chooser) const;

    /**
     * Returns true if the given chooser is assigned to the given choice.
     */
    [[nodiscard]] bool is_in_choice(int chooser, int choice) const;

    /**
     * Returns the maximum preference that any chooser has for any of their assigned to choice.
     */
    [[nodiscard]] int max_used_preference() const;

    /**
     * Returns the input data this instance is an assignment solution for.
     */
    [[nodiscard]] InputData const& input_data() const;

    bool operator == (Assignment const& other) const;
    bool operator != (Assignment const& other) const;
};


