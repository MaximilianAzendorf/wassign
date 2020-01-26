#pragma once

#include <algorithm>
#include <climits>

#include "Types.h"
#include "WorkshopData.h"
#include "ParticipantData.h"
#include "SlotData.h"
#include "Constraint.h"
#include "MutableInputData.h"
#include "UnionFind.h"
#include "Constraints.h"
#include "InputException.h"

class InputData
{
private:
    vector<WorkshopData> _workshops;
    vector<ParticipantData> _participants;
    vector<SlotData> _slots;
    vector<Constraint> _schedulingConstraints;
    vector<Constraint> _assignmentConstraints;
    vector<vector<int>> _dependentWorkshopGroups;
    vector<int> _preferenceLevels;
    int _maxPreference;

    map<int, vector<Constraint>> _workshopConstraintMap;
    map<int, vector<Constraint>> _participantConstraintMap;

    MutableInputData _mutableData;

    vector<pair<int, int>> get_dependent_workshop_limits(vector<Constraint> const& constraints)
    {
        UnionFind<int> workshopGroups(_workshops.size());

        for(Constraint const& constraint : constraints)
        {
            if (constraint.type() != WorkshopsHaveSameParticipants) continue;
            workshopGroups.join(constraint.left(), constraint.right());
        }

        vector<pair<int, int>> limits(_workshops.size(), std::make_pair(0, INT_MAX));

        for(int w = 0; w < _workshops.size(); w++)
        {
            int idx = workshopGroups.find(w);
            limits[idx] = std::make_pair(
                    std::max(limits[idx].first, _workshops[w].min()),
                    std::min(limits[idx].second, _workshops[w].max()));
        }

        for(int w = 0; w < _workshops.size(); w++)
        {
            limits[w] = limits[workshopGroups.find(w)];
        }

        return limits;
    }

    vector<vector<int>> get_dependent_preferences(vector<Constraint> const& constraints)
    {
        vector<vector<int>> depGroups = Constraints::get_dependent_workshops(constraints, workshop_count());

        vector<vector<int>> pref;

        for(int p = 0; p < _participants.size(); p++)
        {
            pref.push_back(_participants[p].preferences());

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

    void compute_conductor_constraints(vector<Constraint>& constraints)
    {
        map<int, vector<int>> conductorMap;

        for(auto data : _mutableData.conductors)
        {
            constraints.push_back(Constraint(ParticipantIsInWorkshop, data.participant, data.workshop));
            conductorMap[data.participant].push_back(data.workshop);
        }

        for(auto conductorList : conductorMap)
        {
            if(conductorList.second.size() <= 1) continue;
            for(int i = 0; i < conductorList.second.size(); i++)
            {
                for(int j = i + 1; j < conductorList.second.size(); j++)
                {
                    constraints.push_back(Constraint(WorkshopsAreNotInSameSlot, conductorList.second[i], conductorList.second[j]));
                }
            }
        }
    }

    void compute_part_constraints(vector<Constraint>& constraints)
    {
        UnionFind<int> workshopGroups(_workshops.size());

        for(int w = 0; w < _workshops.size(); w++)
        {
            if(_workshops[w].has_continuation())
            {
                workshopGroups.join(w, _workshops[w].continuation());
            }
        }

        for(vector<int> group : workshopGroups.groups())
        {
            for(int i = 0; i < group.size(); i++)
            {
                for(int j = i + 1; j < group.size(); j++)
                {
                    constraints.push_back(Constraint(WorkshopsHaveSameParticipants, group[i], group[j]));
                    constraints.push_back(Constraint(WorkshopsHaveOffset, group[i], group[j], j - i));
                }
            }
        }
    }

    template<typename ConstraintParserFunction>
    vector<Constraint> parse_constraints(ConstraintParserFunction constraintBuilder)
    {
        vector<Constraint> constraints;

        for(string const& extraConstraint : _mutableData.constraintStrings)
        {
            for(Constraint constraint : constraintBuilder(*this, extraConstraint))
            {
                constraints.push_back(constraint);
            }
        }

        compute_conductor_constraints(constraints);
        compute_part_constraints(constraints);

        return constraints;
    }

    void build_constraint_maps()
    {
        for(int i = 0; i < _workshops.size(); i++)
        {
            _workshopConstraintMap[i] = {};
        }

        for(int i = 0; i < _participants.size(); i++)
        {
            _participantConstraintMap[i] = {};
        }

        for(Constraint const& constraint : _schedulingConstraints)
        {
            switch(constraint.type())
            {
                case WorkshopIsInSlot:
                case WorkshopIsNotInSlot:
                {
                    _workshopConstraintMap[constraint.left()].push_back(constraint);
                    break;
                }
                case WorkshopsAreInSameSlot:
                case WorkshopsAreNotInSameSlot:
                case WorkshopsHaveOffset:
                {
                    _workshopConstraintMap[constraint.left()].push_back(constraint);
                    _workshopConstraintMap[constraint.right()].push_back(constraint);
                    break;
                }
                case ParticipantIsInWorkshop:
                case ParticipantIsNotInWorkshop:
                {
                    _participantConstraintMap[constraint.left()].push_back(constraint);
                    break;
                }
                case ParticipantsHaveSameWorkshops:
                {
                    _participantConstraintMap[constraint.left()].push_back(constraint);
                    _participantConstraintMap[constraint.right()].push_back(constraint);
                    break;
                }
                case WorkshopsHaveSameParticipants:
                {
                    for(int i = 0; i < _participants.size(); i++)
                    {
                        _participantConstraintMap[i].push_back(constraint);
                    }
                    break;
                }
                case SlotHasLimitedSize:
                {
                    for(int i = 0; i < _workshops.size(); i++)
                    {
                        _workshopConstraintMap[i].push_back(constraint);
                    }
                    break;
                }
                default: throw std::logic_error("Unknown constraint type " + str(constraint.type()) + ".");
            }
        }
    }

public:
    inline static const string GeneratedPrefix = "~";
    inline static const string NotScheduledSlotPrefix = GeneratedPrefix + "not_scheduled_";
    inline static const string HiddenWorkshopPrefix = GeneratedPrefix + "hidden_";
    inline static const string GeneratedSlotName = "Generated Slot";
    inline static const int MinPrefPlaceholder = INT_MAX;

    explicit InputData(MutableInputData& data)
    {
        _mutableData = data;

        _maxPreference = INT_MIN;
        for(int p = 0; p < data.participants.size(); p++)
        {
            for(int w = 0; w < data.workshops.size(); w++)
            {
                int pref = -data.participants[p].preference(w);
                _maxPreference = std::max(_maxPreference, pref);
            }
        }

        for(int p = 0; p < data.participants.size(); p++)
        {
            vector<int> newPrefs(data.participants[p].preferences());
            for(int i = 0; i < newPrefs.size(); i++)
            {
                newPrefs[i] = newPrefs[i] == MinPrefPlaceholder ? 0 : (newPrefs[i] + _maxPreference);
            }

            data.participants[p] = ParticipantData(data.participants[p].name(), newPrefs);
        }

        for(int p = 0; p < data.participants.size(); p++)
        {
            for(int w = 0; w < data.workshops.size(); w++)
            {
                _preferenceLevels.push_back(data.participants[p].preference(w));
            }
        }

        std::sort(_preferenceLevels.begin(), _preferenceLevels.end());
        _preferenceLevels.erase(std::unique(_preferenceLevels.begin(), _preferenceLevels.end()), _preferenceLevels.end());

        _workshops = data.workshops;
        _participants = data.participants;
        _slots = data.slots;

        if(_slots.empty())
        {
            _slots.push_back(SlotData(GeneratedSlotName));
        }
    }

    template<typename ConstraintParserFunction>
    void build_constraints(ConstraintParserFunction constraintParser)
    {
        vector<Constraint> constraints(_mutableData.constraints);

        for(Constraint constraint : parse_constraints(constraintParser))
        {
            constraints.push_back(constraint);
        }

        bool isInfeasible = false;
        constraints = Constraints::reduce_and_optimize(constraints, workshop_count(), isInfeasible);

        if(isInfeasible)
        {
            throw InputException("The given constraints are not satisfiable.");
        }

        auto newLimits = get_dependent_workshop_limits(constraints);
        auto newPrefs = get_dependent_preferences(constraints);

        for(int i = 0; i < _workshops.size(); i++)
        {
            _workshops[i] = WorkshopData(
                    _workshops[i].name(),
                    newLimits[i].first,
                    newLimits[i].second,
                    _workshops[i].opt_continuation());
        }

        for(int i = 0; i < _participants.size(); i++)
        {
            _participants[i] = ParticipantData(
                    _participants[i].name(),
                    newPrefs[i]);
        }

        for(Constraint& constraint : constraints)
        {
            if(constraint.is_scheduling_constraint())
            {
                _schedulingConstraints.push_back(constraint);
            }
            if(constraint.is_assignment_constraint())
            {
                _assignmentConstraints.push_back(constraint);
            }
        }

        build_constraint_maps();
        _dependentWorkshopGroups = Constraints::get_dependent_workshops(constraints, workshop_count());
    }

    [[nodiscard]] int preference_after(int preference) const
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

    [[nodiscard]] vector<Constraint> const& scheduling_constraints() const { return _schedulingConstraints; }
    [[nodiscard]] vector<Constraint> const& scheduling_constraints(int workshopId) const { return _workshopConstraintMap.at(workshopId); }

    [[nodiscard]] vector<Constraint> const& assignment_constraints() const { return _assignmentConstraints; }
    [[nodiscard]] vector<Constraint> const& assignment_constraints(int participantId) const { return _participantConstraintMap.at(participantId); }

    [[nodiscard]] vector<WorkshopData> const& workshops() const { return _workshops; }

    [[nodiscard]] vector<ParticipantData> const& participants() const { return _participants; }

    [[nodiscard]] vector<SlotData> const& slots() const { return _slots; }

    [[nodiscard]] WorkshopData const& workshop(int index) const { return _workshops[index]; }

    [[nodiscard]] ParticipantData const& participant(int index) const { return _participants[index]; }

    [[nodiscard]] SlotData const& slot(int index) const { return _slots[index]; }

    [[nodiscard]] vector<vector<int>> const& dependent_workshop_groups() const { return _dependentWorkshopGroups; }

    [[nodiscard]] vector<int> const& preference_levels() const { return _preferenceLevels; }

    [[nodiscard]] int max_preference() const { return _maxPreference; }

    [[nodiscard]] int workshop_count() const { return _workshops.size(); }

    [[nodiscard]] int participant_count() const { return _participants.size(); }

    [[nodiscard]] int slot_count() const { return _slots.size(); }
};