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
#include "SlotData.h"
#include "Constraint.h"

#include <climits>

class InputDataBuilder;

/**
 * Contains all necessary input data after all parsing and preprocessing steps. Instances of this class are not
 * constructed directly but are built by InputDataBuilder.
 */
class InputData
{
    friend class InputDataBuilder;

private:
    InputData() = default;

    vector<ChoiceData> _choices;
    vector<ChooserData> _choosers;
    vector<SlotData> _slots;
    vector<Constraint> _schedulingConstraints;
    vector<Constraint> _assignmentConstraints;
    vector<vector<int>> _dependentChoiceGroups;
    vector<int> _preferenceLevels;
    int _maxPreference = -1;

    map<int, vector<Constraint>> _choiceConstraintMap;
    map<int, vector<Constraint>> _chooserConstraintMap;

public:
    inline static const string GeneratedPrefix = "~";
    inline static const string NotScheduledSlotPrefix = GeneratedPrefix + "not_scheduled_";
    inline static const string HiddenChoicePrefix = GeneratedPrefix + "hidden_";
    inline static const string GeneratedSlotName = "Generated Slot";
    inline static const int MinPrefPlaceholder = INT_MAX;

    /**
     * Returns the next higher preference after the given preference occuring in the input data.
     */
    [[nodiscard]] int preference_after(int preference) const;

    /**
     * Returns all scheduling constraints.
     */
    [[nodiscard]] vector<Constraint> const& scheduling_constraints() const;

    /**
     * Returns all scheduling constraints relevant for the given choice.
     */
    [[nodiscard]] vector<Constraint> const& scheduling_constraints(int choiceId) const;

    /**
     * Returns all assignment constraints.
     */
    [[nodiscard]] vector<Constraint> const& assignment_constraints() const;

    /**
     * Returns all assignment constraints relevant for the given chooser.
     */
    [[nodiscard]] vector<Constraint> const& assignment_constraints(int chooserId) const;

    /**
     * Returns all choices.
     */
    [[nodiscard]] vector<ChoiceData> const& choices() const;

    /**
     * Returns all choosers.
     */
    [[nodiscard]] vector<ChooserData> const& choosers() const;

    /**
     * Returns all slots.
     */
    [[nodiscard]] vector<SlotData> const& slots() const;

    /**
     * Returns the choice with the given ID.
     */
    [[nodiscard]] ChoiceData const& choice(int index) const;

    /**
     * Returns the chooser with the given ID.
     */
    [[nodiscard]] ChooserData const& chooser(int index) const;

    /**
     * Returns the slot with the given ID.
     */
    [[nodiscard]] SlotData const& slot(int index) const;

    /**
     * Returns all dependent choice groups. For more info see Constraints::get_dependent_choices.
     */
    [[nodiscard]] vector<vector<int>> const& dependent_choice_groups() const;

    /**
     * Returns all preference levels occuring in the input.
     */
    [[nodiscard]] vector<int> const& preference_levels() const;

    /**
     * Returns the maximum preference occuring in the input.
     */
    [[nodiscard]] int max_preference() const;

    /**
     * Returns the number of choices in the input.
     */
    [[nodiscard]] int choice_count() const;

    /**
     * Returns the number of choosers in the input.
     */
    [[nodiscard]] int chooser_count() const;

    /**
     * Returns the number of slots in the input.
     */
    [[nodiscard]] int slot_count() const;
};