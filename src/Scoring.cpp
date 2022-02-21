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

#include "Scoring.h"

#include <cmath>

#include "Util.h"

bool Scoring::satisfies_constraints_scheduling(Scheduling const& scheduling)
{
    for(Constraint constraint : scheduling.input_data().scheduling_constraints())
    {
        int l = constraint.left();
        int r = constraint.right();
        int e = constraint.extra();

        switch(constraint.type())
        {
            case ChoiceIsInSlot:
                if(scheduling.slot_of(l) != r) return false;
                break;

            case ChoiceIsNotInSlot:
                if(scheduling.slot_of(l) == r) return false;
                break;

            case ChoicesAreInSameSlot:
                if(scheduling.slot_of(l) != scheduling.slot_of(r)) return false;
                break;

            case ChoicesAreNotInSameSlot:
                if(scheduling.slot_of(l) == Scheduling::NOT_SCHEDULED
                    || scheduling.slot_of(r) == Scheduling::NOT_SCHEDULED)
                {
                    break;
                }
                if(scheduling.slot_of(l) == scheduling.slot_of(r)) return false;
                break;

            case ChoicesHaveOffset:
                if(scheduling.slot_of(l) == Scheduling::NOT_SCHEDULED
                   || scheduling.slot_of(r) == Scheduling::NOT_SCHEDULED)
                {
                    break;
                }
                if(scheduling.slot_of(r) - scheduling.slot_of(l) != e) return false;
                break;

            case SlotHasLimitedSize:
            {
                int count = 0;
                for(int w = 0; w < scheduling.input_data().choice_count(); w++)
                {
                    if(scheduling.slot_of(w) == constraint.left())
                    {
                        count++;
                    }
                }

                switch((SlotSizeLimitOp)e)
                {
                    case Eq: if(count != r) return false; break;
                    case Neq: if(count == r) return false; break;
                    case Gt: if(count <= r) return false; break;
                    case Lt: if(count >= r) return false; break;
                    case Geq: if(count < r) return false; break;
                    case Leq: if(count > r) return false; break;
                    default: throw std::logic_error("Unknown slot size limit operator " + str(e) + ".");
                }
                break;
            }

            default: throw std::logic_error("Unknown scheduling constraint type " + str(constraint.type()) + ".");
        }
    }

    return true;
}

bool Scoring::satisfies_constraints_assignment(Scheduling const& scheduling, Assignment const& assignment)
{
    for(Constraint const& constraint : assignment.input_data().assignment_constraints())
    {
        int l = constraint.left();
        int r = constraint.right();
        //int e = constraint.extra();

        switch(constraint.type())
        {
            case ChoicesHaveSameChoosers:
                if(scheduling.slot_of(r) == Scheduling::NOT_SCHEDULED || scheduling.slot_of(l) == Scheduling::NOT_SCHEDULED) continue;
                if(assignment.choosers_ordered(l) != assignment.choosers_ordered(r)) return false;
                break;

            case ChooserIsInChoice:
                if(scheduling.slot_of(r) == Scheduling::NOT_SCHEDULED) continue;
                if(!assignment.is_in_choice(l, r)) return false;
                break;

            case ChooserIsNotInChoice:
                if(assignment.is_in_choice(l, r)) return false;
                break;

            case ChoosersHaveSameChoices:
                if(assignment.choices_ordered(l) != assignment.choices_ordered(r)) return false;
                break;

            default: throw std::logic_error("Unknown assignment constraint type " + str(constraint.type()) + ".");
        }
    }

    return true;
}

bool Scoring::satisfies_constraints(Solution const& solution) const
{
    return satisfies_constraints_scheduling(*solution.scheduling())
        && satisfies_constraints_assignment(*solution.scheduling(), *solution.assignment());
}

int Scoring::evaluate_major(Solution const& solution) const
{
    int m = 0;
    for(int i = 0; i < _inputData->chooser_count() * _inputData->slot_count(); i++)
    {
        int p = i / _inputData->slot_count();
        int ws = solution.assignment()->choice_of(p, i % _inputData->slot_count());
        m = std::max(m, _inputData->chooser(p).preferences[ws]);
    }

    return m;
}

float Scoring::evaluate_minor(Solution const& solution) const
{
    if(!is_feasible(solution))
    {
        return INFINITY;
    }

    vector<int> prefCount(_inputData->max_preference() + 1);

    for(int i = 0; i < _inputData->chooser_count() * _inputData->slot_count(); i++)
    {
        int p = i / _inputData->slot_count();
        int ws = solution.assignment()->choice_of(p, i % _inputData->slot_count());
        prefCount[_inputData->chooser(p).preferences[ws]]++;
    }

    float sum = 0;
    for(int pref = 0; pref <= _inputData->max_preference(); pref++)
    {
        sum += prefCount[pref] * std::pow((float)pref, (float)_options->preference_exponent()) / _scaling;
    }

    return sum;
}

Scoring::Scoring(const_ptr<InputData> inputData, const_ptr<Options> options)
        : _inputData(std::move(inputData)),
        _options(std::move(options))
{
    _scaling = std::pow((float)_inputData->max_preference(), (float)_options->preference_exponent());
}

bool Scoring::is_feasible(Solution const& solution) const
{
    vector<int> partCounts(_inputData->choice_count());
    vector<vector<bool>> isInSlot(_inputData->chooser_count(), vector<bool>(_inputData->slot_count()));
    vector<int> slots(_inputData->choice_count());

    if(!satisfies_constraints(solution))
    {
        return false;
    }

    for(int i = 0; i < _inputData->choice_count(); i++)
    {
        slots[i] = solution.scheduling()->slot_of(i);
    }

    for(int i = 0; i < _inputData->chooser_count() * _inputData->slot_count(); i++)
    {
        int p = i / _inputData->slot_count();
        int ws = solution.assignment()->choice_of(p, i % _inputData->slot_count());
        if(isInSlot[p][slots[ws]])
        {
            return false;
        }

        isInSlot[p][slots[ws]] = true;
        partCounts[ws]++;
    }

    for(int i = 0; i < _inputData->choice_count(); i++)
    {
        if (solution.scheduling()->slot_of(i) == Scheduling::NOT_SCHEDULED)
        {
            continue;
        }

        if(partCounts[i] < _inputData->choice(i).min || partCounts[i] > _inputData->choice(i).max)
        {
            return false;
        }
    }

    return true;
}

Score Scoring::evaluate(Solution const& solution) const
{
    if(solution.is_invalid())
    {
        return {.major = INFINITY, .minor = INFINITY};
    }

    auto major = _options->greedy() ? NAN : (float)evaluate_major(solution);
    auto minor = (float)evaluate_minor(solution);

    if((std::isfinite(major) || std::isnan(major)) && std::isfinite(minor))
    {
        return {.major = major, .minor = minor};
    }
    else
    {
        return {.major = INFINITY, .minor = INFINITY};
    }
}
