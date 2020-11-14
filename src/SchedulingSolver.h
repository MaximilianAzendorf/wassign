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
     * The available max push is the sum of the maximum participant counts of all workshops that are not yet assigned
     * to a slot (the maximum number of participants that can be covered with all workshops that are not
     * yet assigned to a slot).
     */
    int calculate_available_max_push(vector<int> const& workshopScramble, int depth);

    /**
     * Tests if the current partial solution satisfies all given critical sets.
     */
    bool satisfies_critical_sets(map<int, int> const& decisions, vector<CriticalSet> const& criticalSets);

    /**
     * Tests if the hypothetical decision of putting workshop into slot would violate any scheduling constraints.
     */
    bool satisfies_scheduling_constraints(int workshop, int slot, map<int, int> const& decisions);

    /**
     * Tests if the current partial solution has any impossibilities. Impossibilities are slots that contain so few
     * workshops that even with all the workshops not yet assigned they would not have enough capacity for all
     * participants.
     */
    bool has_impossibilities(map<int, int> const& decisions, int availableMaxPush);

    /**
     * Calculates critical slots in the current partial solution that limit the next decision. Critical slots are slots
     * that need the next workshop in order to still be able to fulfill the participant count.
     */
    vector<int> calculate_critical_slots(map<int, int> const& decisions, int availableMaxPush, int workshop);

    /**
     * Calculates the score used to decide the order in which slots are preferred when deciding a slot for a workshop
     * (smaller score means higher priority). Currently, the score is the sum of the maximum participant counts of all
     * workshops in this slot.
     */
    int slot_order_heuristic_score(map<int, int> const& decisions, int slot);

    /**
     * Calculates the slots that are feasible for the next workshop regarding the current partial solution. A slot is
     * infeasible if adding the workshop would cause the minimum participant count to exceed the total number of
     * participants.
     *
     * @param lowPrioritySlot A list of slots that are low priority (they should be tried last while backtracking).
     */
    vector<int> calculate_feasible_slots(map<int, int> const& decisions, vector<bool> const& lowPrioritySlot, int workshop);

    /**
     * Shuffles the list of workshops to randomize the solutions found first. Currently, only auto-generated slots for
     * unscheduled workshops are low priority.
     * @return
     */
    vector<int> get_workshop_scramble();

    /**
     * Generates a vector v where v[n]=true means that n is a low-priority workshop.
     */
    vector<bool> get_low_priority_slots();

    /**
     * Transforms the decision map into list of lists d, where d[n] is the list of workshops assigned to slot n.
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
