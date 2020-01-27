#pragma once

#include "Types.h"
#include "Scheduling.h"
#include "CriticalSetAnalysis.h"

class SchedulingSolver
{
private:
    InputData const* _inputData;
    CriticalSetAnalysis _csAnalysis;
    shared_ptr<Scheduling const> _currentSolution;
    bool _hasSolution;

    int calculate_available_max_push(vector<int> const& workshopScramble, int depth);

    bool satisfies_critical_sets(map<int, int> const& decisions, vector<CriticalSet> const& criticalSets);

    bool satisfies_scheduling_constraints(int workshop, int slot, map<int, int> const& decisions);

    bool has_impossibilities(map<int, int> const& decisions, int availableMaxPush);

    vector<int> calculate_critical_slots(map<int, int> const& decisions, int availableMaxPush, int workshop);

    int slot_order_heuristic_score(map<int, int> const& decisions, int slot);

    vector<int> calculate_feasible_slots(map<int, int> const& decisions, vector<bool> const& lowPrioritySlot, int workshop);

    vector<int> get_workshop_scramble();

    vector<bool> get_low_priority_slot_map();

    vector<vector<int>> convert_decisions(map<int, int> const& decisions);

    vector<vector<int>> solve_scheduling(vector<CriticalSet> const& criticalSets, datetime timeLimit);

public:
    inline static const int PREF_RELAXATION = 10;

    SchedulingSolver(InputData const& inputData, CriticalSetAnalysis csAnalysis);

    bool next_scheduling();

    [[nodiscard]] shared_ptr<Scheduling const> scheduling()
    {
        return _currentSolution;
    }

    [[nodiscard]] bool has_solution() const
    {
        return _hasSolution;
    }
};
