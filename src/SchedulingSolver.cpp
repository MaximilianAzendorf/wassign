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

int SchedulingSolver::calculate_available_max_push(vector<int> const& choiceScramble, int depth)
{
    int push = 0;
    for(; depth < choiceScramble.size(); depth++)
    {
        push += _inputData->choice(choiceScramble[depth]).max();
    }

    return push;
}

bool SchedulingSolver::satisfies_critical_sets(map<int, int> const& decisions, vector<CriticalSet> const& criticalSets)
{
    set<int> coveredSets;
    for(CriticalSet const& set : criticalSets)
    {
        coveredSets.clear();
        int missing = 0;

        for(int element : set.elements())
        {
            auto setIt = decisions.find(element);

            if(setIt == decisions.end())
            {
                missing++;
            }
            else
            {
                coveredSets.insert(setIt->second);
            }
        }

        if(coveredSets.size() + missing < _inputData->set_count())
        {
            return false;
        }
    }

    return true;
}

bool SchedulingSolver::satisfies_scheduling_constraints(int choice, int set, map<int, int> const& decisions)
{
    for(Constraint constraint : _inputData->scheduling_constraints(choice))
    {
        switch(constraint.type())
        {
            case ChoiceIsInSet: if(set != constraint.right()) return false; break;
            case ChoiceIsNotInSet: if(set == constraint.right()) return false; break;

            case ChoicesAreInSameSet:
            {
                int other = constraint.left() == choice ? constraint.right() : constraint.left();
                auto otherSetIt = decisions.find(other);
                if(otherSetIt != decisions.end() && otherSetIt->second != set)
                {
                    return false;
                }
                break;
            }

            case ChoicesAreNotInSameSet:
            {
                int other = constraint.left() == choice ? constraint.right() : constraint.left();
                auto otherSetIt = decisions.find(other);
                if(otherSetIt != decisions.end() && otherSetIt->second == set)
                {
                    return false;
                }
                break;
            }

            case ChoicesHaveOffset:
            {
                int other = constraint.left() == choice ? constraint.right() : constraint.left();
                int offset = other == constraint.left() ? -constraint.extra() : constraint.extra();
                auto otherSetIt = decisions.find(other);
                if(otherSetIt != decisions.end() && otherSetIt->second - set != offset)
                {
                    return false;
                }

                int minSet = std::max(0, 0 - offset);
                int maxSet = std::min(_inputData->set_count(), _inputData->set_count() - offset);

                if(set < minSet || set >= maxSet) return false;
                break;
            }

            case SetHasLimitedSize:
            {
                // Todo: Implement better constraint infeasibility detection for this constraint type.
                //
                if(constraint.left() != set) continue;
                if(constraint.extra() == Neq || constraint.extra() == Gt || constraint.extra() == Geq) break;

                int limit = constraint.left() - (constraint.extra() == Lt ? 1 : 0);

                for(auto const& decision : decisions)
                {
                    if(decision.second == set) limit--;
                }

                if(limit < 0) return false;

                break;
            }

            default: throw std::logic_error("Unknown scheduling type " + str(constraint.type()) + ".");
        }
    }

    return true;
}

bool SchedulingSolver::has_impossibilities(map<int, int> const& decisions, int availableMaxPush)
{
    for(int s = 0; s < _inputData->set_count(); s++)
    {
        int sum = availableMaxPush;
        for(auto const& decision : decisions)
        {
            if(decision.second != s) continue;
            sum += _inputData->choice(decision.first).max();
        }

        if(sum < _inputData->chooser_count())
        {
            return true;
        }
    }

    return false;
}

vector<int>
SchedulingSolver::calculate_critical_sets(map<int, int> const& decisions, int availableMaxPush, int choice)
{
    vector<int> criticalSets;

    for(int s = 0; s < _inputData->set_count(); s++)
    {
        int sum = availableMaxPush - _inputData->choice(choice).max();
        for(auto const& decision : decisions)
        {
            if(decision.second != s) continue;
            sum += _inputData->choice(decision.first).max();
        }

        if(sum >= _inputData->chooser_count() || !satisfies_scheduling_constraints(choice, s, decisions))
        {
            continue;
        }

        criticalSets.push_back(s);
    }

    return criticalSets;
}

int SchedulingSolver::set_order_heuristic_score(map<int, int> const& decisions, int set)
{
    int score = 0;
    for(auto const& decision : decisions)
    {
        if(decision.second != set) continue;
        score += _inputData->choice(decision.first).max();
    }

    return score;
}

vector<int>
SchedulingSolver::calculate_feasible_sets(map<int, int> const& decisions, vector<bool> const& lowPrioritySet, int choice)
{
    // Feasible sets are all sets for which adding the current choice would not cause the
    // minimal chooser number of this set to exceed the total chooser count.
    //
    // We then have to filter the feasible set by all additional constraints.
    //
    // We order the feasible sets by the maximal chooser number as a heuristic to get more
    // balanced schedulings.
    //
    vector<int> normalSets, lowSets;
    vector<int> setScore(_inputData->set_count(), INT_MIN);

    for(int s = 0; s < _inputData->set_count(); s++)
    {
        int sum = _inputData->choice(choice).min();
        for(auto const& decision : decisions)
        {
            if(decision.second != s) continue;
            sum += _inputData->choice(decision.first).min();
        }

        if(sum > _inputData->chooser_count() || !satisfies_scheduling_constraints(choice, s, decisions))
        {
            continue;
        }

        if (lowPrioritySet[s])
        {
            lowSets.push_back(s);
        }
        else
        {
            normalSets.push_back(s);
            setScore[s] = set_order_heuristic_score(decisions, s);
        }
    }

    std::sort(normalSets.begin(), normalSets.end(),
              [&](int const& s1, int const& s2) { return setScore[s1] < setScore[s2]; });

    return riffle_shuffle(normalSets, lowSets);
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

vector<bool> SchedulingSolver::get_low_priority_sets()
{
    vector<bool> lowPrioritySet(_inputData->set_count());
    for(int s = 0; s < _inputData->set_count(); s++)
    {
        lowPrioritySet[s] = _inputData->set(s).name().rfind(InputData::NotScheduledSetPrefix, 0) == 0;
    }

    return lowPrioritySet;
}

vector<vector<int>> SchedulingSolver::convert_decisions(map<int, int> const& decisions)
{
    vector<vector<int>> res(_inputData->set_count());
    for(auto const& decision : decisions)
    {
        res[decision.second].push_back(decision.first);
    }

    return res;
}

vector<vector<int>> SchedulingSolver::solve_scheduling(vector<CriticalSet> const& criticalSets, datetime timeLimit)
{
    vector<int> choiceScramble = get_choice_scramble();
    vector<bool> lowPrioritySet = get_low_priority_sets();

    map<int, int> decisions;
    stack<vector<int>> backtracking;

    for(int depth = 0; depth < choiceScramble.size();)
    {
        if(is_set(_cancellation)) return {};
        if(time_now() > timeLimit)
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
                // example, for 4 Sets and the critical set {A, B, C, D, E}, a partial solution of the form
                //
                //      Set 1:    ... A, C ....
                //      Set 2:    ... D .......
                //      Set 3:    .............
                //      Set 4:    .............
                //   Not assigned: ... B, E ....
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
                    vector<int> criticalSets = calculate_critical_sets(decisions, availableMaxPush, choice);

                    if(criticalSets.size() == 1)
                    {
                        backtracking.push(criticalSets);
                    }
                    else if(criticalSets.size() > 1)
                    {
                        backtracking.push({});
                    }
                    else
                    {
                        vector<int> feasibleSets = calculate_feasible_sets(decisions, lowPrioritySet, choice);
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

    vector<vector<int>> sets;

    while(sets.empty())
    {
        vector<CriticalSet> csSets = _csAnalysis->for_preference(preferenceLimit);

        datetime timeLimit =  preferenceLimit == _inputData->max_preference()
                              ? time_never()
                              : time_now() + seconds(_options->critical_set_timeout_seconds());

        sets = solve_scheduling(csSets, timeLimit);

        if(sets.empty())
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

    vector<int> scheduling(_inputData->choice_count());
    for(int s = 0; s < sets.size(); s++)
    {
        for(int w = 0; w < sets[s].size(); w++)
        {
            scheduling[sets[s][w]] = s;
        }
    }

    _hasSolution = true;
    _currentSolution = std::make_shared<Scheduling>(_inputData, scheduling);

    return true;
}
