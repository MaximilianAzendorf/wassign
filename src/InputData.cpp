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
#include "InputException.h"

vector<pair<int, int>> InputData::get_dependent_choice_limits(vector<Constraint> const& constraints)
{
    UnionFind<int> choiceGroups(_choices.size());

    for(Constraint const& constraint : constraints)
    {
        if (constraint.type() != ChoicesHaveSameChoosers) continue;
        choiceGroups.join(constraint.left(), constraint.right());
    }

    vector<pair<int, int>> limits(_choices.size(), std::make_pair(0, INT_MAX));

    for(int w = 0; w < _choices.size(); w++)
    {
        int idx = choiceGroups.find(w);
        limits[idx] = std::make_pair(
                std::max(limits[idx].first, _choices[w].min()),
                std::min(limits[idx].second, _choices[w].max()));
    }

    for(int w = 0; w < _choices.size(); w++)
    {
        limits[w] = limits[choiceGroups.find(w)];
    }

    return limits;
}

vector<vector<int>> InputData::get_dependent_preferences(vector<Constraint> const& constraints)
{
    vector<vector<int>> depGroups = Constraints::get_dependent_choices(constraints, choice_count());

    vector<vector<int>> pref;

    for(int p = 0; p < _choosers.size(); p++)
    {
        pref.push_back(_choosers[p].preferences());

        for(vector<int> const& group : depGroups)
        {
            int min = INT_MAX;

            for(int w : group)
            {
                min = std::min(min, pref[p][w]);
            }

            for(int w : group)
            {
                pref[p][w] = min;
            }
        }
    }

    return pref;
}

void InputData::compute_conductor_constraints(vector<Constraint>& constraints)
{
    map<int, vector<int>> conductorMap;

    for(auto data : _mutableData.conductors)
    {
        constraints.push_back(Constraint(ChooserIsInChoice, data.chooser, data.choice));
        conductorMap[data.chooser].push_back(data.choice);
    }

    for(auto conductorList : conductorMap)
    {
        if(conductorList.second.size() <= 1) continue;
        for(int i = 0; i < conductorList.second.size(); i++)
        {
            for(int j = i + 1; j < conductorList.second.size(); j++)
            {
                constraints.push_back(Constraint(ChoicesAreNotInSameSet, conductorList.second[i], conductorList.second[j]));
            }
        }
    }
}

void InputData::compute_part_constraints(vector<Constraint>& constraints)
{
    UnionFind<int> choiceGroups(_choices.size());

    for(int w = 0; w < _choices.size(); w++)
    {
        if(_choices[w].has_continuation())
        {
            choiceGroups.join(w, _choices[w].continuation());
        }
    }

    for(vector<int> group : choiceGroups.groups())
    {
        // TODO: Implement proper choice order support for choice series.
        //
        std::sort(group.begin(), group.end());
        for(int i = 0; i < group.size(); i++)
        {
            for(int j = i + 1; j < group.size(); j++)
            {
                constraints.push_back(Constraint(ChoicesHaveSameChoosers, group[i], group[j]));
                constraints.push_back(Constraint(ChoicesHaveOffset, group[i], group[j], j - i));
            }
        }
    }
}

void InputData::build_constraint_maps()
{
    for(int i = 0; i < _choices.size(); i++)
    {
        _choiceConstraintMap[i] = {};
    }

    for(int i = 0; i < _choosers.size(); i++)
    {
        _chooserConstraintMap[i] = {};
    }

    for(Constraint const& constraint : _schedulingConstraints)
    {
        switch(constraint.type())
        {
            case ChoiceIsInSet:
            case ChoiceIsNotInSet:
            {
                _choiceConstraintMap[constraint.left()].push_back(constraint);
                break;
            }
            case ChoicesAreInSameSet:
            case ChoicesAreNotInSameSet:
            case ChoicesHaveOffset:
            {
                _choiceConstraintMap[constraint.left()].push_back(constraint);
                _choiceConstraintMap[constraint.right()].push_back(constraint);
                break;
            }
            case ChooserIsInChoice:
            case ChooserIsNotInChoice:
            {
                _chooserConstraintMap[constraint.left()].push_back(constraint);
                break;
            }
            case ChoosersHaveSameChoices:
            {
                _chooserConstraintMap[constraint.left()].push_back(constraint);
                _chooserConstraintMap[constraint.right()].push_back(constraint);
                break;
            }
            case ChoicesHaveSameChoosers:
            {
                for(int i = 0; i < _choosers.size(); i++)
                {
                    _chooserConstraintMap[i].push_back(constraint);
                }
                break;
            }
            case SetHasLimitedSize:
            {
                for(int i = 0; i < _choices.size(); i++)
                {
                    _choiceConstraintMap[i].push_back(constraint);
                }
                break;
            }
            default: throw std::logic_error("Unknown constraint type " + str(constraint.type()) + ".");
        }
    }
}

InputData::InputData(MutableInputData& data)
{
    _mutableData = data;

    _maxPreference = INT_MIN;
    for(int p = 0; p < data.choosers.size(); p++)
    {
        if(data.choosers[p].preferences().size() != data.choices.size())
        {
            throw InputException("Wrong number of preferences given for chooser \"" + data.choosers[p].name() + "\".");
        }
        for(int w = 0; w < data.choices.size(); w++)
        {
            int pref = -data.choosers[p].preference(w);
            _maxPreference = std::max(_maxPreference, pref);
        }
    }

    for(int p = 0; p < data.choosers.size(); p++)
    {
        vector<int> newPrefs(data.choosers[p].preferences());
        for(int i = 0; i < newPrefs.size(); i++)
        {
            newPrefs[i] = newPrefs[i] == MinPrefPlaceholder ? 0 : (newPrefs[i] + _maxPreference);
        }

        data.choosers[p] = ChooserData(data.choosers[p].name(), newPrefs);
    }

    for(int p = 0; p < data.choosers.size(); p++)
    {
        for(int w = 0; w < data.choices.size(); w++)
        {
            _preferenceLevels.push_back(data.choosers[p].preference(w));
        }
    }
    _preferenceLevels.push_back(_maxPreference);

    std::sort(_preferenceLevels.begin(), _preferenceLevels.end());
    _preferenceLevels.erase(std::unique(_preferenceLevels.begin(), _preferenceLevels.end()), _preferenceLevels.end());

    _choices = data.choices;
    _choosers = data.choosers;
    _sets = data.sets;

    if(_sets.empty())
    {
        _sets.push_back(SetData(GeneratedSetName));
    }
}

int InputData::preference_after(int preference) const
{
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

vector<SetData> const& InputData::sets() const
{
    return _sets;
}

ChoiceData const& InputData::choice(int index) const
{
    return _choices[index];
}

ChooserData const& InputData::chooser(int index) const
{
    return _choosers[index];
}

SetData const& InputData::set(int index) const
{
    return _sets[index];
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

int InputData::set_count() const
{
    return _sets.size();
}
