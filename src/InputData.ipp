#pragma once

#include "InputData.h"

#include "Constraints.h"
#include "InputException.h"

template<typename ConstraintParserFunction>
vector<Constraint> InputData::parse_constraints(ConstraintParserFunction constraintBuilder)
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

template<typename ConstraintParserFunction>
void InputData::build_constraints(ConstraintParserFunction constraintParser)
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