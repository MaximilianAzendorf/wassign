#pragma once

#include <memory>
#include <utility>

#include "Types.h"
#include "Scheduling.h"
#include "CriticalSetAnalysis.h"
#include "Rng.h"

class SchedulingSolver
{
private:
    InputData const* _inputData;
    CriticalSetAnalysis _csAnalysis;
    shared_ptr<Scheduling const> _currentSolution;
    bool _hasSolution;

    int calculate_available_max_push(vector<int> const& workshopScramble, int depth)
    {
        // The maximum number of participants that can be covered with all workshops that are not yet assigned
        // to a slot.
        //
        int push = 0;
        for(; depth < workshopScramble.size(); depth++)
        {
            push += _inputData->workshop(workshopScramble[depth]).max();
        }

        return push;
    }

    bool satisfies_critical_sets(map<int, int> const& decisions, vector<CriticalSet> const& criticalSets)
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
                else
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

    bool satisfies_scheduling_constraints(int workshop, int slot, map<int, int> const& decisions)
    {
        for(Constraint constraint : _inputData->scheduling_constraints(workshop))
        {
            switch(constraint.type())
            {
                case WorkshopIsInSlot: if(slot != constraint.right()) return false; break;
                case WorkshopIsNotInSlot: if(slot == constraint.right()) return false; break;

                case WorkshopsAreInSameSlot:
                {
                    int other = constraint.left() == workshop ? constraint.right() : constraint.left();
                    auto otherSlotIt = decisions.find(other);
                    if(otherSlotIt != decisions.end() && otherSlotIt->second != slot)
                    {
                        return false;
                    }
                    break;
                }

                case WorkshopsAreNotInSameSlot:
                {
                    int other = constraint.left() == workshop ? constraint.right() : constraint.left();
                    auto otherSlotIt = decisions.find(other);
                    if(otherSlotIt != decisions.end() && otherSlotIt->second == slot)
                    {
                        return false;
                    }
                    break;
                }

                case WorkshopsHaveOffset:
                {
                    int other = constraint.left() == workshop ? constraint.right() : constraint.left();
                    int offset = other == constraint.left() ? -constraint.extra() : constraint.extra();
                    auto otherSlotIt = decisions.find(other);
                    if(otherSlotIt != decisions.end() && otherSlotIt->second - slot != offset)
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
                    // Todo: Implement better constraint infeasibility detection for this constraint type.
                    //
                    if(constraint.left() != slot) continue;
                    if(constraint.extra() == Neq || constraint.extra() == Gt || constraint.extra() == Geq) break;

                    int limit = constraint.left() - (constraint.extra() == Lt ? 1 : 0);

                    for(auto const& decision : decisions)
                    {
                        if(decision.second == slot) limit--;
                    }

                    if(limit < 0) return false;

                    break;
                }

                default: throw std::logic_error("Unknown scheduling type " + str(constraint.type()) + ".");
            }
        }

        return true;
    }

    bool has_impossibilities(map<int, int> const& decisions, int availableMaxPush)
    {
        // Impossibilities are slots that contain so few workshops that even with all the workshops not yet
        // assigned they would not have enough capacity for all participants.
        //
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            int sum = availableMaxPush;
            for(auto const& decision : decisions)
            {
                if(decision.second != s) continue;
                sum += _inputData->workshop(decision.first).max();
            }

            if(sum < _inputData->participant_count())
            {
                return true;
            }
        }

        return false;
    }

    vector<int> calculate_critical_slots(map<int, int> const& decisions, int availableMaxPush, int workshop)
    {
        // Critical slots are slots that need the current workshop in order to still be able
        // to fulfill the participant count.
        //
        vector<int> criticalSlots;

        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            int sum = availableMaxPush - _inputData->workshop(workshop).max();
            for(auto const& decision : decisions)
            {
                if(decision.second != s) continue;
                sum += _inputData->workshop(decision.first).max();
            }

            if(sum >= _inputData->participant_count() || !satisfies_scheduling_constraints(workshop, s, decisions))
            {
                continue;
            }

            criticalSlots.push_back(s);
        }

        return criticalSlots;
    }

    int slot_order_heuristic_score(map<int, int> const& decisions, int slot)
    {
        int score = 0;
        for(auto const& decision : decisions)
        {
            if(decision.second != slot) continue;
            score += _inputData->workshop(decision.first).max();
        }

        return score;
    }

    vector<int> calculate_feasible_slots(map<int, int> const& decisions, vector<bool> const& lowPrioritySlot, int workshop)
    {
        // Feasible slots are all slots for which adding the current workshop would not cause the
        // minimal participant number of this slot to exceed the total participant count.
        //
        // We then have to filter the feasible slot by all additional constraints.
        //
        // We order the feasible slots by the maximal participant number as a heuristic to get more
        // balanced schedulings.
        //
        vector<int> normalSlots, lowSlots;
        vector<int> slotScore(_inputData->slot_count(), INT_MIN);

        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            int sum = _inputData->workshop(workshop).min();
            for(auto const& decision : decisions)
            {
                if(decision.second != s) continue;
                sum += _inputData->workshop(decision.first).min();
            }

            if(sum > _inputData->participant_count() || !satisfies_scheduling_constraints(workshop, s, decisions))
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

    vector<int> get_workshop_scramble()
    {
        vector<int> workshopScramble(_inputData->workshop_count());
        std::iota(workshopScramble.begin(), workshopScramble.end(), 0);
        std::shuffle(workshopScramble.begin(), workshopScramble.end(), Rng::engine());
        std::sort(workshopScramble.begin(), workshopScramble.end(), [&](int const& x, int const& y)
        {
            return _inputData->scheduling_constraints(x).size() > _inputData->scheduling_constraints(y).size();
        });

        return workshopScramble;
    }

    vector<bool> get_low_priority_slot_map()
    {
        vector<bool> lowPrioritySlot(_inputData->slot_count());
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            lowPrioritySlot[s] = _inputData->slot(s).name().rfind(InputData::NotScheduledSlotPrefix, 0) == 0;
        }

        return lowPrioritySlot;
    }

    vector<vector<int>> convert_decisions(map<int, int> const& decisions)
    {
        vector<vector<int>> res(_inputData->slot_count());
        for(auto const& decision : decisions)
        {
            res[decision.second].push_back(decision.first);
        }

        return res;
    }

    vector<vector<int>> solve_scheduling(vector<CriticalSet> const& criticalSets, datetime timeLimit)
    {
        vector<int> workshopScramble = get_workshop_scramble();
        vector<bool> lowPrioritySlot = get_low_priority_slot_map();

        map<int, int> decisions;
        stack<vector<int>> backtracking;

        for(int depth = 0; depth < workshopScramble.size();)
        {
            boost::this_thread::interruption_point();
            if(time_now() > timeLimit)
            {
                return {};
            }

            int workshop = workshopScramble[depth];

            if(backtracking.size() <= depth)
            {
                int availableMaxPush = calculate_available_max_push(workshopScramble, depth);

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
                    // This is the case when there aren't enough elements in a critical set to cover all slots. For
                    // example, for 4 Slots and the critical set {A, B, C, D}, a partial solution of the form
                    //
                    //      Slot 1:    ... A, C ....
                    //      Slot 2:    ... D .......
                    //      Slot 3:    .............
                    //      Slot 4:    .............
                    //   Not assigned: ... B .......
                    //
                    // Would not be feasible, because the critical set can not be covered anymore (we would need at
                    // least 2 open workshops in the critical set to cover Slot 3 and 4).
                    //
                    if(!satisfies_critical_sets(decisions, criticalSets))
                    {
                        backtracking.push({});
                    }
                    else
                    {
                        vector<int> criticalSlots = calculate_critical_slots(decisions, availableMaxPush, workshop);

                        if(criticalSlots.size() == 1)
                        {
                            backtracking.push(criticalSlots);
                        }
                        else if(criticalSlots.size() > 1)
                        {
                            backtracking.push({});
                        }
                        else
                        {
                            vector<int> feasibleSlots = calculate_feasible_slots(decisions, lowPrioritySlot, workshop);
                            backtracking.push(feasibleSlots);
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
                decisions.erase(workshopScramble[depth - 1]);
                depth--;
                continue;
            }

            int nextSlot = backtracking.top().front();
            auto& top = backtracking.top();
            top.erase(std::remove(top.begin(), top.end(), nextSlot), top.end());
            decisions[workshop] = nextSlot;
            depth++;
        }

        return convert_decisions(decisions);
    }

public:
    inline static const int PREF_RELAXATION = 10;

    SchedulingSolver(InputData const& inputData, CriticalSetAnalysis csAnalysis)
        : _inputData(&inputData),
          _csAnalysis(std::move(csAnalysis)),
          _currentSolution(new Scheduling(inputData)),
          _hasSolution(false)
    {
    }

    bool next_scheduling()
    {
        int preferenceLimit = Rng::next(0, PREF_RELAXATION) == 0
                ? _inputData->max_preference()
                : _csAnalysis.preference_bound();

        vector<vector<int>> slots;

        while(slots.empty())
        {
            vector<CriticalSet> csSets = _csAnalysis.for_preference(preferenceLimit);

            datetime timeLimit =  preferenceLimit == _inputData->max_preference()
                    ? time_never()
                    : time_now() + seconds(Options::critical_set_timeout_seconds());

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

        vector<int> scheduling(_inputData->workshop_count());
        for(int s = 0; s < slots.size(); s++)
        {
            for(int w = 0; w < slots[s].size(); w++)
            {
                scheduling[slots[s][w]] = s;
            }
        }

        _hasSolution = true;
        _currentSolution = std::make_shared<Scheduling>(*_inputData, scheduling);

        return true;
    }

    [[nodiscard]] shared_ptr<Scheduling const> scheduling()
    {
        return _currentSolution;
    }

    [[nodiscard]] bool has_solution() const
    {
        return _hasSolution;
    }
};
