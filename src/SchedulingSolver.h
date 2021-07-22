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

#include <future>
#include "Types.h"
#include "Scheduling.h"
#include "CriticalSetAnalysis.h"
#include "Options.h"

/**
 * Class for calculating (randomized) valid schedulings, taking critical sets into account.
 */
class SchedulingSolver
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<CriticalSetAnalysis> _csAnalysis;

    const_ptr<Scheduling> _currentSolution;
    bool _hasSolution;

    const_ptr<Options> _options;
    cancel_token _cancellation;

    /**
     * The available max push is the sum of the maximum chooser counts of all choices that are not yet assigned
     * to a set (the maximum number of choosers that can be covered with all choices that are not
     * yet assigned to a set).
     */
    int calculate_available_max_push(vector<int> const& choiceScramble, int depth);

    /**
     * Tests if the current partial solution satisfies all given critical sets.
     */
    bool satisfies_critical_sets(map<int, int> const& decisions, vector<CriticalSet> const& criticalSets);

    /**
     * Tests if the hypothetical decision of putting choice into set would violate any scheduling constraints.
     */
    bool satisfies_scheduling_constraints(int choice, int set, map<int, int> const& decisions);

    /**
     * Checks if the hypothetical decision of putting choice into set would violate any slot size constraints. This
     * method must only be called when the given hypothetical decision will be the last decision before the scheduling
     * is complete.
     */
    bool check_slot_size_constraints([[maybe_unused]] int choice, int slot, map<int, int> const& decisions);

    /**
     * Tests if the current partial solution has any impossibilities. Impossibilities are sets that contain so few
     * choices that even with all the choices not yet assigned they would not have enough capacity for all
     * choosers.
     */
    bool has_impossibilities(map<int, int> const& decisions, int availableMaxPush);

    /**
     * Calculates critical sets in the current partial solution that limit the next decision. Critical sets are sets
     * that need the next choice in order to still be able to fulfill the chooser count.
     */
    vector<int> calculate_critical_slots(map<int, int> const& decisions, int availableMaxPush, int choice);

    /**
     * Calculates the score used to decide the order in which sets are preferred when deciding a set for a choice
     * (smaller score means higher priority). Currently, the score is the sum of the maximum chooser counts of all
     * choices in this set.
     */
    int slot_order_heuristic_score(map<int, int> const& decisions, int set);

    /**
     * Calculates the sets that are feasible for the next choice regarding the current partial solution. A set is
     * infeasible if adding the choice would cause the minimum chooser count to exceed the total number of
     * choosers.
     *
     * @param lowPrioritySet A list of sets that are low priority (they should be tried last while backtracking).
     */
    vector<int> calculate_feasible_slots(map<int, int> const& decisions, vector<bool> const& lowPrioritySet, int choice);

    /**
     * Shuffles the list of choices to randomize the solutions found first. Currently, only auto-generated sets for
     * unscheduled choices are low priority.
     * @return
     */
    vector<int> get_choice_scramble();

    /**
     * Generates a vector v where v[n]=true means that n is a low-priority slot.
     */
    vector<bool> get_low_priority_slots();

    /**
     * Transforms the decision map into list of lists d, where d[n] is the list of choices assigned to set n.
     */
    vector<vector<int>> convert_decisions(map<int, int> const& decisions);

    /**
     * Solves a scheduling. If the timeLimit is reached, an empty vector is returned.
     */
    vector<vector<int>> solve_scheduling(vector<CriticalSet> const& criticalSets, datetime timeLimit);

public:
    /**
     * With a chance of 1/PREF_RELAXATION, the solver will search for a solution disregarding critical sets. This is to
     * avoid being locked into a very small solution space by a very restrictive (but valid) critical set subset, which
     * would hamper hill climbing performance.
     */
    inline static const int PREF_RELAXATION = 10;

    /**
     * Constructor.
     */
    SchedulingSolver(const_ptr<InputData> inputData,
                     const_ptr<CriticalSetAnalysis> csAnalysis,
                     const_ptr<Options> options,
                     cancel_token cancellation = cancel_token());

    /**
     * Tries to calculate the next scheduling and returns false if none is found (within the time limit).
     */
    bool next_scheduling();

    /**
     * Returns the last found solution.
     */
    [[nodiscard]] const_ptr<Scheduling> scheduling()
    {
        return _currentSolution;
    }

    /**
     * Returns true if the solver found at least one solution in the past.
     */
    [[nodiscard]] bool has_solution() const
    {
        return _hasSolution;
    }
};
