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

    void copy_data(InputReader const& reader);

    void build_preferences(InputReader const& reader);

    void build_preference_levels(InputReader const& reader);

    /**
     * Converts all InputChoiceData instances into final choice descriptions.
     */
    void compile_choices(InputReader const& reader);

    /**
     * Generates extra sets if there are optional choices present (which are internally handled with a virtual
     * "not scheduled"-set and extra virtual workhops to which to assign all choosers in this virtual set).
     */
    void generate_extra_sets(InputReader const& reader);

    vector<Constraint> parse_constraints(InputReader const& reader);

    void build_constraints(InputReader const& reader);

    vector<pair<int, int>> get_dependent_choice_limits(vector<Constraint> const& constraints);

    vector<vector<int>> get_dependent_preferences(vector<Constraint> const& constraints);

    void compute_part_constraints(vector<Constraint>& constraints);

    void build_constraint_maps();

public:
    const_ptr<InputData> get_input_data() const;

    void process_input_reader(InputReader const& reader);
};

