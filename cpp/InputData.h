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

    vector<pair<int, int>> dependent_workshop_limits(vector<Constraint> const& constraints)
    {
        // TODO: Implement
        throw std::logic_error("Not implemented");
    }

    vector<vector<int>> dependent_preferences(vector<Constraint> const& constraints)
    {
        // TODO: Implement
        throw std::logic_error("Not implemented");
    }

    vector<Constraint> build_constraints()
    {
        vector<Constraint> constraints;

        // TODO: Acutally build constraints here

        auto newLimits = dependent_workshop_limits(constraints);
        auto newPreferences = dependent_preferences(constraints);

        for(int i = 0; i < workshop_count(); i++)
        {
            _workshops[i] = WorkshopData(
                    _workshops[i].name(),
                    std::get<0>(newLimits[i]),
                    std::get<1>(newLimits[i]),
                    _workshops[i].opt_continuation());
        }

        for(int i = 0; i < participant_count(); i++)
        {
            _participants[i] = ParticipantData(
                    _participants[i].name(),
                    newPreferences[i]);
        }

        return constraints;
    }

    void build_constraint_maps()
    {
        // TODO: Implement
        throw std::logic_error("Not implemented");
    }

    void build_dependent_workshop_groups()
    {
        // TODO: Implement
        throw std::logic_error("Not implemented");
    }

public:
    inline static const string GeneratedPrefix = "~";
    inline static const string NotScheduledSlotPrefix = GeneratedPrefix + "not_scheduled_";
    inline static const string HiddenWorkshopPrefix = GeneratedPrefix + "hidden_";
    inline static const string GeneratedSlotName = "Generated Slot";

    InputData(MutableInputData const& data, bool buildConstraints = true)
    {
        _workshops = data.workshops;
        _participants = data.participants;
        _slots = data.slots;

        if(_slots.empty())
        {
            _slots.push_back(SlotData(GeneratedSlotName));
        }

        vector<Constraint> constraints;
        if(buildConstraints)
        {
            constraints = build_constraints();
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
        build_dependent_workshop_groups();

        _maxPreference = INT_MIN;
        for(auto& participant : _participants)
        {
            for(int w = 0; w < workshop_count(); w++)
            {
                int pref = participant.preference(w);
                _preferenceLevels.push_back(pref);
                _maxPreference = std::max(_maxPreference, pref);
            }
        }

        _preferenceLevels.erase(std::unique(_preferenceLevels.begin(), _preferenceLevels.end()), _preferenceLevels.end());
        std::sort(_preferenceLevels.begin(), _preferenceLevels.end());
    }

    [[nodiscard]] int preference_after(int preference) const
    {
        for(int i = 0; i < _preferenceLevels.size(); i++)
        {
            if(_preferenceLevels[i] > preference)
            {
                return i;
            }
        }

        return INT_MAX;
    }

    [[nodiscard]] vector<Constraint> const& scheduling_constraints() const { return _schedulingConstraints; }
    [[nodiscard]] vector<Constraint> const& scheduling_constraints(int workshopId) const { return _workshopConstraintMap[workshopId]; }

    [[nodiscard]] vector<Constraint> const& assignment_constraints() const { return _assignmentConstraints; }
    [[nodiscard]] vector<Constraint> const& assignment_constraints(int participantId) const { return _participantConstraintMap[participantId]; }

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