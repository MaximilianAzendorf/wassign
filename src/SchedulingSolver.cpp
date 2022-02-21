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

#include "SchedulingSolver.h"

#include <memory>
#include <utility>

#include "Rng.h"
#include "Util.h"
#include "Options.h"
#include "Scheduling.h"

int SchedulingSolver::calculate_available_max_push(vector<int> const& choiceScramble, int depth)
{
    int push = 0;
    for(; depth < choiceScramble.size(); depth++)
    {
        push += _inputData->choice(choiceScramble[depth]).max;
    }

    return push;
}

bool SchedulingSolver::satisfies_critical_sets(map<int, int> const& decisions, vector<CriticalSet> const& criticalSets)
{
    set<int> coveredSlots;
    for(CriticalSet const& set : criticalSets)
    {
        coveredSlots.clear();
        int missing = 0;

        for(int element : set.elements())
        {
            auto slotIt = decisions.find(element);

            if(slotIt == decisions.end())
            {
                missing++;
            }
            else if (slotIt->second != Scheduling::NOT_SCHEDULED)
            {
                coveredSlots.insert(slotIt->second);
            }
        }

        if(coveredSlots.size() + missing < _inputData->slot_count())
        {
            return false;
        }
    }

    return true;
}

bool SchedulingSolver::satisfies_scheduling_constraints(int choice, int slot, map<int, int> const& decisions)
{
    for(Constraint constraint : _inputData->scheduling_constraints(choice))
    {
        switch(constraint.type())
        {
            case ChoiceIsInSlot: if(slot != constraint.right()) return false; break;
            case ChoiceIsNotInSlot: if(slot == constraint.right()) return false; break;

            case ChoicesAreInSameSlot:
            {
                int other = constraint.left() == choice ? constraint.right() : constraint.left();
                auto otherSlotIt = decisions.find(other);
                if(otherSlotIt == decisions.end()) break;

                if(otherSlotIt->second != slot)
                {
                    return false;
                }
                break;
            }

            case ChoicesAreNotInSameSlot:
            {
                int other = constraint.left() == choice ? constraint.right() : constraint.left();
                auto otherSlotIt = decisions.find(other);
                if (otherSlotIt == decisions.end()) break;

                if(otherSlotIt->second == slot && slot != Scheduling::NOT_SCHEDULED)
                {
                    return false;
                }
                break;
            }

            case ChoicesHaveOffset:
            {
                int other = constraint.left() == choice ? constraint.right() : constraint.left();
                int offset = other == constraint.left() ? -constraint.extra() : constraint.extra();
                auto otherSlotIt = decisions.find(other);
                if(otherSlotIt == decisions.end()) break;

                if((otherSlotIt->second == Scheduling::NOT_SCHEDULED) != (slot == Scheduling::NOT_SCHEDULED))
                {
                    // Choices that have an offset must either both be scheduled or both not be scheduled.
                    return false;
                }
                if(slot == Scheduling::NOT_SCHEDULED)
                {
                    // The offset does not matter if the choice is not scheduled.
                    break;
                }
                if(otherSlotIt->second - slot != offset)
                {
                    return false;
                }

                int minSlot = std::max(0, 0 - offset);
                int maxSlot = std::min(_inputData->slot_count(), _inputData->slot_count() - offset);

                if(slot < minSlot || slot >= maxSlot) return false;
                break;
            }

            case SlotHasLimitedSize:
            {
                if(constraint.left() != slot) continue;

                // These will only be checked at the end of the backtracking (for now).
                // Todo: Maybe implement earlier constraint infeasibility detection Gt and Geq.
                //
                if(constraint.extra() == Neq || constraint.extra() == Gt || constraint.extra() == Geq) break;

                int limit = constraint.right() - (constraint.extra() == Lt ? 1 : 0);

                for(auto const& decision : decisions)
                {
                    if(decision.second == slot) limit--;
                }

                if(limit < 1) return false;

                break;
            }

            default: throw std::logic_error("Unknown scheduling type " + str(constraint.type()) + ".");
        }
    }

    if(decisions.size() + 1 == _inputData->choice_count())
    {
        // This is the last decision to be made, time to check slot size constraints.
        //
        return check_slot_size_constraints(choice, slot, decisions);
    }
    else
    {
        return true;
    }
}

bool SchedulingSolver::check_slot_size_constraints([[maybe_unused]] int choice, int slot, map<int, int> const& decisions)
{
    if (slot < 0) return true;

    // these will be counted lazily so we don't have to compute slot sizes if there are no slot size constraints.
    //
    vector<int> slotSizes;

    for(Constraint constraint : _inputData->scheduling_constraints())
    {
        if(constraint.type() != SlotHasLimitedSize) continue;

        if(slotSizes.empty())
        {
            slotSizes.resize(_inputData->slot_count());
            slotSizes[slot]++;
            for(int w = 0; w < _inputData->choice_count(); w++)
            {
                if(choice == w) continue;
                int wslot = decisions.at(w);
                if (wslot != Scheduling::NOT_SCHEDULED)
                {
                    slotSizes[wslot]++;
                }
            }
        }

        bool valid = false;
        switch(constraint.extra())
        {
            case Eq: valid = slotSizes[constraint.left()] == constraint.right(); break;
            case Neq: valid = slotSizes[constraint.left()] != constraint.right(); break;
            case Lt: valid = slotSizes[constraint.left()] < constraint.right(); break;
            case Leq: valid = slotSizes[constraint.left()] <= constraint.right(); break;
            case Gt: valid = slotSizes[constraint.left()] > constraint.right(); break;
            case Geq: valid = slotSizes[constraint.left()] >= constraint.right(); break;
        }

        if(!valid) return false;
    }

    return true;
}

bool SchedulingSolver::has_impossibilities(map<int, int> const& decisions, int availableMaxPush)
{
    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        int sum = availableMaxPush;
        for(auto const& decision : decisions)
        {
            if(decision.second != s) continue;
            sum += _inputData->choice(decision.first).max;
        }

        if(sum < _inputData->chooser_count())
        {
            return true;
        }
    }

    return false;
}

vector<int>
SchedulingSolver::calculate_critical_slots(map<int, int> const& decisions, int availableMaxPush, int choice)
{
    vector<int> criticalSets;

    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        int sum = availableMaxPush - _inputData->choice(choice).max;
        for(auto const& decision : decisions)
        {
            if(decision.second != s) continue;
            sum += _inputData->choice(decision.first).max;
        }

        if(sum >= _inputData->chooser_count())
        {
            continue;
        }

        criticalSets.push_back(s);
    }

    return criticalSets;
}

int SchedulingSolver::slot_order_heuristic_score(map<int, int> const& decisions, int set)
{
    int score = 0;
    for(auto const& decision : decisions)
    {
        if(decision.second != set) continue;
        score += _inputData->choice(decision.first).max;
    }

    return score;
}

vector<int>
SchedulingSolver::calculate_feasible_slots(map<int, int> const& decisions, vector<bool> const& lowPrioritySlot, int choice)
{
    // Feasible slots are all slots for which adding the current choice would not cause the
    // minimal chooser number of this slot to exceed the total chooser count.
    //
    // We then have to filter the feasible slots by all additional constraints.
    //
    // We order the feasible slots by the maximal chooser number as a heuristic to get more
    // balanced schedulings.
    //
    vector<int> normalSlots, lowSlots;
    vector<int> slotScore(_inputData->slot_count(), INT_MIN);

    if (_inputData->choice(choice).isOptional && satisfies_scheduling_constraints(choice, Scheduling::NOT_SCHEDULED, decisions))
    {
        lowSlots.push_back(Scheduling::NOT_SCHEDULED);
    }

    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        int sum = _inputData->choice(choice).min;
        for(auto const& decision : decisions)
        {
            if(decision.second != s) continue;
            sum += _inputData->choice(decision.first).min;
        }

        if(sum > _inputData->chooser_count() || !satisfies_scheduling_constraints(choice, s, decisions))
        {
            continue;
        }

        if (lowPrioritySlot[s])
        {
            lowSlots.push_back(s);
        }
        else
        {
            normalSlots.push_back(s);
            slotScore[s] = slot_order_heuristic_score(decisions, s);
        }
    }

    std::sort(normalSlots.begin(), normalSlots.end(),
              [&](int const& s1, int const& s2) { return slotScore[s1] < slotScore[s2]; });

    return riffle_shuffle(normalSlots, lowSlots);
}

vector<int> SchedulingSolver::get_choice_scramble()
{
    vector<int> choiceScramble(_inputData->choice_count());
    std::iota(choiceScramble.begin(), choiceScramble.end(), 0);
    std::shuffle(choiceScramble.begin(), choiceScramble.end(), Rng::engine());
    std::sort(choiceScramble.begin(), choiceScramble.end(), [&](int const& x, int const& y)
    {
        return _inputData->scheduling_constraints(x).size() > _inputData->scheduling_constraints(y).size();
    });

    return choiceScramble;
}

vector<bool> SchedulingSolver::get_low_priority_slots()
{
    // TODO: Remove this function?
    vector<bool> res(_inputData->slot_count());
    std::fill(res.begin(), res.end(), false);
    return res;
}

vector<vector<int>> SchedulingSolver::convert_decisions(map<int, int> const& decisions)
{
    vector<vector<int>> res(_inputData->slot_count());
    for(auto const& decision : decisions)
    {
        if (decision.second < 0) continue;
        res[decision.second].push_back(decision.first);
    }

    return res;
}

vector<vector<int>> SchedulingSolver::solve_scheduling(vector<CriticalSet> const& criticalSets, datetime timeLimit)
{
    vector<int> choiceScramble = get_choice_scramble();
    vector<bool> lowPrioritySet = get_low_priority_slots();

    map<int, int> decisions;
    stack<vector<int>> backtracking;

    for(int depth = 0; depth < choiceScramble.size();)
    {
        if(time_now() > timeLimit || is_set(_cancellation))
        {
            return {};
        }

        int choice = choiceScramble[depth];

        if(backtracking.size() <= depth)
        {
            int availableMaxPush = calculate_available_max_push(choiceScramble, depth);

            // If there are any impossibilities, the current partial solution is infeasible.
            //
            if(has_impossibilities(decisions, availableMaxPush))
            {
                backtracking.push({});
            }
            else
            {
                // If the partial solution does not satisfy critical set constraints it is infeasible.
                //
                // This is the case when there aren't enough elements in a critical set to cover all sets. For
                // example, for 4 Slots and the critical set {A, B, C, D, E}, a partial solution of the form
                //
                //      Slot 1:    .. A, C, E ..
                //      Slot 2:    .. D ........
                //      Slot 3:    .............
                //      Slot 4:    .............
                //  Not assigned:  .. B ........
                //
                // Would not be feasible, because the critical set can not be covered anymore (we would need at
                // least 2 open choices in the critical set to cover Set 3 and 4).
                //
                if(!satisfies_critical_sets(decisions, criticalSets))
                {
                    backtracking.push({});
                }
                else
                {
                    vector<int> criticalSlots = calculate_critical_slots(decisions, availableMaxPush, choice);

                    if(criticalSlots.size() == 1)
                    {
                        if (satisfies_scheduling_constraints(choice, criticalSlots[0], decisions))
                        {
                            backtracking.push(criticalSlots);
                        }
                        else
                        {
                            backtracking.push({});
                        }
                    }
                    else if(criticalSlots.size() > 1)
                    {
                        backtracking.push({});
                    }
                    else
                    {
                        vector<int> feasibleSets = calculate_feasible_slots(decisions, lowPrioritySet, choice);
                        backtracking.push(feasibleSets);
                    }
                }
            }
        }

        if(backtracking.top().empty())
        {
            if(depth == 0)
            {
                // no solution
                //
                return {};
            }

            // backtrack
            //
            backtracking.pop();
            decisions.erase(choiceScramble[depth - 1]);
            depth--;
            continue;
        }

        int nextSet = backtracking.top().front();
        auto& top = backtracking.top();
        top.erase(std::remove(top.begin(), top.end(), nextSet), top.end());
        decisions[choice] = nextSet;
        depth++;
    }

    return convert_decisions(decisions);
}

SchedulingSolver::SchedulingSolver(const_ptr<InputData> inputData,
                                   const_ptr<CriticalSetAnalysis> csAnalysis,
                                   const_ptr<Options> options,
                                   cancel_token cancellation)
        : _inputData(std::move(inputData)),
          _csAnalysis(std::move(csAnalysis)),
          _currentSolution(new Scheduling(_inputData)),
          _hasSolution(false),
          _options(std::move(options)),
          _cancellation(std::move(cancellation))
{
}

bool SchedulingSolver::next_scheduling()
{
    int preferenceLimit = Rng::next(0, PREF_RELAXATION) == 0
                          ? _inputData->max_preference()
                          : _csAnalysis->preference_bound();

    vector<vector<int>> slots;

    while(slots.empty())
    {
        vector<CriticalSet> csSets = _csAnalysis->for_preference(preferenceLimit);

        datetime timeLimit =  preferenceLimit == _inputData->max_preference()
                              ? time_never()
                              : time_now() + seconds(_options->critical_set_timeout_seconds());

        slots = solve_scheduling(csSets, timeLimit);

        if(slots.empty())
        {
            if(preferenceLimit == _inputData->max_preference())
            {
                _hasSolution = false;
                _currentSolution = nullptr;
                return false;
            }

            preferenceLimit = _inputData->preference_after(preferenceLimit);
        }
    }

    vector<int> scheduling(_inputData->choice_count(), -1);
    for(int s = 0; s < slots.size(); s++)
    {
        for(int w = 0; w < slots[s].size(); w++)
        {
            scheduling[slots[s][w]] = s;
        }
    }

    _hasSolution = true;
    _currentSolution = std::make_shared<Scheduling>(_inputData, scheduling);

    return true;
}
