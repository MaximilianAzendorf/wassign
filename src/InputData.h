#pragma once

#include "Types.h"
#include "WorkshopData.h"
#include "ParticipantData.h"
#include "SlotData.h"
#include "Constraint.h"
#include "MutableInputData.h"

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

    vector<pair<int, int>> get_dependent_workshop_limits(vector<Constraint> const& constraints);

    vector<vector<int>> get_dependent_preferences(vector<Constraint> const& constraints);

    void compute_conductor_constraints(vector<Constraint>& constraints);

    void compute_part_constraints(vector<Constraint>& constraints);

    template<typename ConstraintParserFunction>
    vector<Constraint> parse_constraints(ConstraintParserFunction constraintBuilder);

    void build_constraint_maps();

public:
    inline static const string GeneratedPrefix = "~";
    inline static const string NotScheduledSlotPrefix = GeneratedPrefix + "not_scheduled_";
    inline static const string HiddenWorkshopPrefix = GeneratedPrefix + "hidden_";
    inline static const string GeneratedSlotName = "Generated Slot";
    inline static const int MinPrefPlaceholder = INT_MAX;

    explicit InputData(MutableInputData& data);

    template<typename ConstraintParserFunction>
    void build_constraints(ConstraintParserFunction constraintParser);

    [[nodiscard]] int preference_after(int preference) const;

    [[nodiscard]] vector<Constraint> const& scheduling_constraints() const;

    [[nodiscard]] vector<Constraint> const& scheduling_constraints(int workshopId) const;

    [[nodiscard]] vector<Constraint> const& assignment_constraints() const;

    [[nodiscard]] vector<Constraint> const& assignment_constraints(int participantId) const;

    [[nodiscard]] vector<WorkshopData> const& workshops() const;

    [[nodiscard]] vector<ParticipantData> const& participants() const;

    [[nodiscard]] vector<SlotData> const& slots() const;

    [[nodiscard]] WorkshopData const& workshop(int index) const;

    [[nodiscard]] ParticipantData const& participant(int index) const;

    [[nodiscard]] SlotData const& slot(int index) const;

    [[nodiscard]] vector<vector<int>> const& dependent_workshop_groups() const;

    [[nodiscard]] vector<int> const& preference_levels() const;

    [[nodiscard]] int max_preference() const;

    [[nodiscard]] int workshop_count() const;

    [[nodiscard]] int participant_count() const;

    [[nodiscard]] int slot_count() const;
};

#include "InputData.ipp"