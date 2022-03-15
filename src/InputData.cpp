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

#include "InputData.h"

#include <algorithm>
#include <climits>

#include "Util.h"
#include "UnionFind.h"
#include "Constraints.h"
#include "input/InputException.h"

int InputData::preference_after(int preference) const
{
    // TODO: Maybe do this with binary search.
    for(int i = 0; i < _preferenceLevels.size(); i++)
    {
        if(_preferenceLevels[i] > preference)
        {
            return _preferenceLevels[i];
        }
    }

    return INT_MAX;
}

vector<Constraint> const& InputData::scheduling_constraints() const
{
    return _schedulingConstraints;
}

vector<Constraint> const& InputData::scheduling_constraints(int choiceId) const
{
    return _choiceConstraintMap.at(choiceId);
}

vector<Constraint> const& InputData::assignment_constraints() const
{
    return _assignmentConstraints;
}

vector<Constraint> const& InputData::assignment_constraints(int chooserId) const
{
    return _chooserConstraintMap.at(chooserId);
}

vector<ChoiceData> const& InputData::choices() const
{
    return _choices;
}

vector<ChooserData> const& InputData::choosers() const
{
    return _choosers;
}

vector<SlotData> const& InputData::slots() const
{
    return _slots;
}

ChoiceData const& InputData::choice(int index) const
{
    return _choices[index];
}

ChooserData const& InputData::chooser(int index) const
{
    return _choosers[index];
}

SlotData const& InputData::slot(int index) const
{
    return _slots[index];
}

vector<vector<int>> const& InputData::dependent_choice_groups() const
{
    return _dependentChoiceGroups;
}

vector<int> const& InputData::preference_levels() const
{
    return _preferenceLevels;
}

int InputData::max_preference() const
{
    return _maxPreference;
}

int InputData::choice_count() const
{
    return _choices.size();
}

int InputData::chooser_count() const
{
    return _choosers.size();
}

int InputData::slot_count() const
{
    return _slots.size();
}
