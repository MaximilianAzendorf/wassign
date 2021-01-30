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
#include "ChoiceData.h"
#include "ChooserData.h"
#include "SetData.h"
#include "Constraint.h"

#include <climits>

class InputDataBuilder;

class InputData
{
    friend class InputDataBuilder;

private:
    InputData() = default;

    vector<ChoiceData> _choices;
    vector<ChooserData> _choosers;
    vector<SetData> _sets;
    vector<Constraint> _schedulingConstraints;
    vector<Constraint> _assignmentConstraints;
    vector<vector<int>> _dependentChoiceGroups;
    vector<int> _preferenceLevels;
    int _maxPreference;

    map<int, vector<Constraint>> _choiceConstraintMap;
    map<int, vector<Constraint>> _chooserConstraintMap;

public:
    inline static const string GeneratedPrefix = "~";
    inline static const string NotScheduledSetPrefix = GeneratedPrefix + "not_scheduled_";
    inline static const string HiddenChoicePrefix = GeneratedPrefix + "hidden_";
    inline static const string GeneratedSetName = "Generated Set";
    inline static const int MinPrefPlaceholder = INT_MAX;

    [[nodiscard]] int preference_after(int preference) const;

    [[nodiscard]] vector<Constraint> const& scheduling_constraints() const;

    [[nodiscard]] vector<Constraint> const& scheduling_constraints(int choiceId) const;

    [[nodiscard]] vector<Constraint> const& assignment_constraints() const;

    [[nodiscard]] vector<Constraint> const& assignment_constraints(int chooserId) const;

    [[nodiscard]] vector<ChoiceData> const& choices() const;

    [[nodiscard]] vector<ChooserData> const& choosers() const;

    [[nodiscard]] vector<SetData> const& sets() const;

    [[nodiscard]] ChoiceData const& choice(int index) const;

    [[nodiscard]] ChooserData const& chooser(int index) const;

    [[nodiscard]] SetData const& set(int index) const;

    [[nodiscard]] vector<vector<int>> const& dependent_choice_groups() const;

    [[nodiscard]] vector<int> const& preference_levels() const;

    [[nodiscard]] int max_preference() const;

    [[nodiscard]] int choice_count() const;

    [[nodiscard]] int chooser_count() const;

    [[nodiscard]] int set_count() const;
};