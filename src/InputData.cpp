#include "InputData.h"

#include <algorithm>
#include <climits>

#include "Util.h"
#include "UnionFind.h"
#include "Constraints.h"
#include "InputException.h"

vector<pair<int, int>> InputData::get_dependent_workshop_limits(vector<Constraint> const& constraints)
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

vector<vector<int>> InputData::get_dependent_preferences(vector<Constraint> const& constraints)
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

void InputData::compute_conductor_constraints(vector<Constraint>& constraints)
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

void InputData::compute_part_constraints(vector<Constraint>& constraints)
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

void InputData::build_constraint_maps()
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

InputData::InputData(MutableInputData& data)
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

int InputData::preference_after(int preference) const
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

vector<Constraint> const& InputData::scheduling_constraints() const
{
    return _schedulingConstraints;
}

vector<Constraint> const& InputData::scheduling_constraints(int workshopId) const
{
    return _workshopConstraintMap.at(workshopId);
}

vector<Constraint> const& InputData::assignment_constraints() const
{
    return _assignmentConstraints;
}

vector<Constraint> const& InputData::assignment_constraints(int participantId) const
{
    return _participantConstraintMap.at(participantId);
}

vector<WorkshopData> const& InputData::workshops() const
{
    return _workshops;
}

vector<ParticipantData> const& InputData::participants() const
{
    return _participants;
}

vector<SlotData> const& InputData::slots() const
{
    return _slots;
}

WorkshopData const& InputData::workshop(int index) const
{
    return _workshops[index];
}

ParticipantData const& InputData::participant(int index) const
{
    return _participants[index];
}

SlotData const& InputData::slot(int index) const
{
    return _slots[index];
}

vector<vector<int>> const& InputData::dependent_workshop_groups() const
{
    return _dependentWorkshopGroups;
}

vector<int> const& InputData::preference_levels() const
{
    return _preferenceLevels;
}

int InputData::max_preference() const
{
    return _maxPreference;
}

int InputData::workshop_count() const
{
    return _workshops.size();
}

int InputData::participant_count() const
{
    return _participants.size();
}

int InputData::slot_count() const
{
    return _slots.size();
}
