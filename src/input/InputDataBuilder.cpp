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
#include "InputDataBuilder.h"
#include "ConstraintBuilder.h"
#include "../UnionFind.h"
#include "../Constraints.h"

vector<pair<int, int>> InputDataBuilder::get_dependent_choice_limits(vector<Constraint> const& constraints)
{
    UnionFind<int> choiceGroups(_inputData->_choices.size());

    for(Constraint const& constraint : constraints)
    {
        if (constraint.type() != ChoicesHaveSameChoosers) continue;
        choiceGroups.join(constraint.left(), constraint.right());
    }

    vector<pair<int, int>> limits(_inputData->_choices.size(), std::make_pair(0, INT_MAX));

    for(int w = 0; w < _inputData->_choices.size(); w++)
    {
        int idx = choiceGroups.find(w);
        limits[idx] = std::make_pair(
                std::max(limits[idx].first, _inputData->_choices[w].min),
                std::min(limits[idx].second, _inputData->_choices[w].max));
    }

    for(int w = 0; w < _inputData->_choices.size(); w++)
    {
        limits[w] = limits[choiceGroups.find(w)];
    }

    return limits;
}

vector<vector<int>> InputDataBuilder::get_dependent_preferences(vector<Constraint> const& constraints)
{
    vector<vector<int>> depGroups = Constraints::get_dependent_choices(constraints, _inputData->choice_count());

    vector<vector<int>> pref;

    for(int p = 0; p < _inputData->_choosers.size(); p++)
    {
        pref.push_back(_inputData->_choosers[p].preferences);

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

void InputDataBuilder::compute_part_constraints(vector<Constraint>& constraints)
{
    UnionFind<int> choiceGroups(_inputData->_choices.size());

    for(int w = 0; w < _inputData->_choices.size(); w++)
    {
        if(_inputData->_choices[w].has_continuation())
        {
            choiceGroups.join(w, _inputData->_choices[w].continuation_value());
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

void InputDataBuilder::build_constraint_maps()
{
    for(int i = 0; i < _inputData->_choices.size(); i++)
    {
        _inputData->_choiceConstraintMap[i] = {};
    }

    for(int i = 0; i < _inputData->_choosers.size(); i++)
    {
        _inputData->_chooserConstraintMap[i] = {};
    }

    for(Constraint const& constraint : _inputData->_schedulingConstraints)
    {
        switch(constraint.type())
        {
            case ChoiceIsInSet:
            case ChoiceIsNotInSet:
            {
                _inputData->_choiceConstraintMap[constraint.left()].push_back(constraint);
                break;
            }
            case ChoicesAreInSameSet:
            case ChoicesAreNotInSameSet:
            case ChoicesHaveOffset:
            {
                _inputData->_choiceConstraintMap[constraint.left()].push_back(constraint);
                _inputData->_choiceConstraintMap[constraint.right()].push_back(constraint);
                break;
            }
            case ChooserIsInChoice:
            case ChooserIsNotInChoice:
            {
                _inputData->_chooserConstraintMap[constraint.left()].push_back(constraint);
                break;
            }
            case ChoosersHaveSameChoices:
            {
                _inputData->_chooserConstraintMap[constraint.left()].push_back(constraint);
                _inputData->_chooserConstraintMap[constraint.right()].push_back(constraint);
                break;
            }
            case ChoicesHaveSameChoosers:
            {
                for(int i = 0; i < _inputData->_choosers.size(); i++)
                {
                    _inputData->_chooserConstraintMap[i].push_back(constraint);
                }
                break;
            }
            case SetHasLimitedSize:
            {
                for(int i = 0; i < _inputData->_choices.size(); i++)
                {
                    _inputData->_choiceConstraintMap[i].push_back(constraint);
                }
                break;
            }
            default: throw std::logic_error("Unknown constraint type " + str(constraint.type()) + ".");
        }
    }
}

void InputDataBuilder::copy_data(InputReader const& reader)
{
    for(auto const& set : reader._sets)
    {
        _inputData->_sets.push_back(*set);
    }

    for(auto const& chooser : reader._choosers)
    {
        _inputData->_choosers.push_back(*chooser);
    }
}

void InputDataBuilder::compile_choices(InputReader const& reader)
{
    int wsidx = 0;

    for(auto const& pw : reader._choices)
    {
        _inputData->_choices.push_back(ChoiceData(
                pw->name,
                pw->min,
                pw->max,
                pw->parts > 1 ? std::make_optional(wsidx + 1) : std::nullopt));

        wsidx++;
        if(pw->parts > 1)
        {
            for(int p = 0; p < _inputData->_choosers.size(); p++)
            {
                vector<int> newPrefs;
                for(int i = 0; i < _inputData->_choosers[p].preferences.size(); i++)
                {
                    for(int j = 0; j < (i == wsidx - 1 ? pw->parts : 1); j++)
                    {
                        newPrefs.push_back(_inputData->_choosers[p].preferences[i]);
                    }
                }

                _inputData->_choosers[p] = ChooserData(_inputData->_choosers[p].name, newPrefs);
            }

            for(int i = 1; i < pw->parts; i++)
            {
                string name = InputData::GeneratedPrefix + "[" + str(i + 1) + "] " + pw->name;
                _inputData->_choices.push_back(ChoiceData(
                        name,
                        pw->min,
                        pw->max,
                        i == pw->parts - 1 ? std::nullopt : std::make_optional(wsidx + 1)));
                wsidx++;
            }
        }
    }
}

void InputDataBuilder::generate_extra_sets(InputReader const& reader)
{
    int optMin = 0;
    bool hasOpt = false;
    for(auto const& pw : reader._choices)
    {
        if(!pw->optional) continue;
        hasOpt = true;
        optMin += pw->min;
    }

    int numExtraSets = (int)std::ceil((double)optMin / (double)_inputData->_choosers.size());
    numExtraSets = std::max(hasOpt ? 1 : 0, numExtraSets);

    for(int i = 0; i < numExtraSets; i++)
    {
        string extraSet = InputData::NotScheduledSetPrefix + str(i);
        string extraChoice = InputData::HiddenChoicePrefix + "unassigned_" + str(i);

        int s = _inputData->_sets.size();

        _inputData->_sets.push_back(SetData(extraSet));
        _inputData->_choices.push_back(ChoiceData(extraChoice, 0, _inputData->_choosers.size() + 1));
        _inputData->_schedulingConstraints.push_back(Constraint(ChoiceIsInSet, _inputData->_choices.size() - 1, s));

        for(int p = 0; p < _inputData->_choosers.size(); p++)
        {
            vector<int> newPref(_inputData->_choosers[p].preferences);
            newPref.push_back(InputData::MinPrefPlaceholder);
            _inputData->_choosers[p] = ChooserData(_inputData->_choosers[p].name, newPref);
        }

        for(auto const& pw : reader._choices)
        {
            if(pw->optional) continue;

            int w = 0;
            for(; w < _inputData->_choices.size(); w++)
            {
                if(_inputData->_choices[w].name == pw->name) break;
            }

            _inputData->_schedulingConstraints.push_back(Constraint(ChoiceIsNotInSet, w, s));
        }
    }
}

vector<Constraint> InputDataBuilder::parse_constraints(InputReader const& reader)
{
    vector<Constraint> constraints;

    for(auto const& extraConstraint : reader._constraintExpressions)
    {
        for(Constraint constraint : ConstraintBuilder::build(*_inputData, extraConstraint))
        {
            constraints.push_back(constraint);
        }
    }

    return constraints;
}

void InputDataBuilder::build_constraints(InputReader const& reader)
{
    vector<Constraint> constraints = parse_constraints(reader);

    compute_part_constraints(constraints);

    bool isInfeasible = false;
    constraints = Constraints::reduce_and_optimize(constraints, _inputData->choice_count(), isInfeasible);

    if(isInfeasible)
    {
        throw InputException("The given constraints are not satisfiable.");
    }

    auto newLimits = get_dependent_choice_limits(constraints);
    auto newPrefs = get_dependent_preferences(constraints);

    for(int i = 0; i < _inputData->_choices.size(); i++)
    {
        _inputData->_choices[i] = ChoiceData(
                _inputData->_choices[i].name,
                newLimits[i].first,
                newLimits[i].second,
                _inputData->_choices[i].continuation);
    }

    for(int i = 0; i < _inputData->_choosers.size(); i++)
    {
        _inputData->_choosers[i] = ChooserData(
                _inputData->_choosers[i].name,
                newPrefs[i]);
    }

    for(Constraint& constraint : constraints)
    {
        if(constraint.is_scheduling_constraint())
        {
            _inputData->_schedulingConstraints.push_back(constraint);
        }
        if(constraint.is_assignment_constraint())
        {
            _inputData->_assignmentConstraints.push_back(constraint);
        }
    }

    build_constraint_maps();
    _inputData->_dependentChoiceGroups = Constraints::get_dependent_choices(constraints, _inputData->choice_count());
}

void InputDataBuilder::build_preferences(InputReader const& reader)
{
    _inputData->_maxPreference = INT_MIN;
    for(int p = 0; p < reader._choosers.size(); p++)
    {
        if(reader._choosers[p]->preferences.size() != reader._choices.size())
        {
            throw InputException("Wrong number of preferences given for chooser \"" + reader._choosers[p]->name + "\".");
        }
        for(int w = 0; w < reader._choices.size(); w++)
        {
            _inputData->_maxPreference = std::max(_inputData->_maxPreference, reader._choosers[p]->preferences[w]);
        }
    }

    for(int p = 0; p < _inputData->_choosers.size(); p++)
    {
        for(int i = 0; i < _inputData->_choosers[p].preferences.size(); i++)
        {
            _inputData->_choosers[p].preferences[i] =
                    _inputData->_maxPreference - _inputData->_choosers[p].preferences[i];
        }
    }
}

void InputDataBuilder::build_preference_levels(InputReader const& reader)
{
    for(int p = 0; p < reader._choosers.size(); p++)
    {
        for(int w = 0; w < reader._choices.size(); w++)
        {
            _inputData->_preferenceLevels.push_back(reader._choosers[p]->preferences[w]);
        }
    }
    _inputData->_preferenceLevels.push_back(_inputData->_maxPreference);
    _inputData->_preferenceLevels.push_back(0);

    std::sort(_inputData->_preferenceLevels.begin(), _inputData->_preferenceLevels.end());
    _inputData->_preferenceLevels.erase(
            std::unique(_inputData->_preferenceLevels.begin(), _inputData->_preferenceLevels.end()
            ), _inputData->_preferenceLevels.end());
}

void InputDataBuilder::process_input_reader(InputReader const& reader)
{
    if(reader._sets.empty())
    {
        _inputData->_sets.push_back(SetData(InputData::GeneratedSetName));
    }

    copy_data(reader);

    build_preferences(reader);
    build_preference_levels(reader);

    compile_choices(reader);
    generate_extra_sets(reader);
    build_constraints(reader);
    build_constraint_maps();
}

const_ptr<InputData> InputDataBuilder::get_input_data() const
{
    return _inputData;
}
