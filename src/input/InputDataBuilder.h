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

#include "../InputData.h"
#include "InputReader.h"

class InputReader;

class InputDataBuilder
{
private:
    shared_ptr<InputData> _inputData = shared_ptr<InputData>(new InputData());

    /**
     * Copies the data from the input reader that don't need any kind of processing.
     */
    void copy_data(InputReader const& reader);

    /**
     * Inverts and normalizes the input preferences (because internally we minimize the score and 0 is the "best"
     * preference).
     */
    void build_preferences(InputReader const& reader);

    /**
     * Builds the preference level vector.
     */
    void build_preference_levels(InputReader const& reader);

    /**
     * Converts all InputChoiceData instances into final choice descriptions.
     */
    void compile_choices(InputReader const& reader);

    /**
     * Generates extra slots if there are optional choices present (which are internally handled with a virtual
     * "not scheduled"-slot and extra virtual workhops to which to assign all choosers in this virtual slot).
     */
    void generate_extra_slots(InputReader const& reader);

    /**
     * Converts the constraint expressions contained int he input reader to constraints.
     */
    vector<Constraint> parse_constraints(InputReader const& reader);

    /**
     * Converts all constraint expressions and build other additional constraints.
     */
    void build_constraints(InputReader const& reader);

    /**
     * Fixes min and max chooser limits of dependent choices because they effectively all have to have the same limits.
     */
    vector<pair<int, int>> get_dependent_choice_limits(vector<Constraint> const& constraints);

    /**
     * Fixes preferences of dependent choices because they have effectively have to be the same for every choice in the
     * dependent set.
     */
    vector<vector<int>> get_dependent_preferences(vector<Constraint> const& constraints);

    /**
     * Generates constraints needed for multi-part choices.
     */
    void compute_part_constraints(vector<Constraint>& constraints);

    /**
     * Builds the constraint maps for efficient constraint lookup.
     */
    void build_constraint_maps();

public:
    const_ptr<InputData> get_input_data() const;

    void process_input_reader(InputReader const& reader);
};

