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

#include "Constraints.h"

#include "Util.h"
#include "UnionFind.h"

vector<vector<int>> Constraints::get_mandatory_critical_sets(vector<Constraint> const& constraints)
{
    vector<vector<int>> res;
    map<int, vector<int>> constGroups;

    for(Constraint const& constraint : constraints)
    {
        if(constraint.type() != ParticipantIsInWorkshop) continue;
        constGroups[constraint.left()].push_back(constraint.right());
    }

    for(auto const& constGroup : constGroups)
    {
        res.push_back(constGroup.second);
    }

    return res;
}

vector<Constraint> Constraints::expand_dependent_constraints(vector<Constraint> const& constraints, int workshopCount)
{
    vector<Constraint> res;

    vector<vector<int>> dependentWorkshops = get_dependent_workshops(constraints, workshopCount);
    vector<vector<int>> mandatoryCritSets = get_mandatory_critical_sets(constraints);

    auto allGroups = {dependentWorkshops, mandatoryCritSets};

    for(auto const& groupList : allGroups)
    {
        for (vector<int> const& group : groupList)
        {
            for(int i = 0; i < group.size(); i++)
            {
                for(int j = i + 1; j < group.size(); j++)
                {
                    res.push_back(Constraint(WorkshopsAreNotInSameSlot, group[i], group[j]));
                }
            }
        }
    }

    for(Constraint const& constraint : constraints)
    {
        if(constraint.type() != ParticipantIsInWorkshop && constraint.type() != ParticipantIsNotInWorkshop) continue;

        vector<int> group;
        for(vector<int> const& depGroup : dependentWorkshops)
        {
            if(std::find(depGroup.begin(), depGroup.end(), constraint.right()) != depGroup.end())
            {
                group = depGroup;
                break;
            }
        }

        if(group.empty()) continue;

        for(int w : group)
        {
            if(w == constraint.right()) continue;
            res.push_back(Constraint(constraint.type(), constraint.left(), w));
        }
    }

    for(Constraint constraint : constraints)
    {
        res.push_back(constraint);
    }

    res.erase(std::unique(res.begin(), res.end()), res.end());
    return res;
}

vector<vector<int>> Constraints::get_dependent_workshops(vector<Constraint> const& constraints, int workshopCount)
{
    UnionFind<int> workshopGroups(workshopCount);

    for(Constraint const& constraint : constraints)
    {
        if(constraint.type() != WorkshopsHaveSameParticipants) continue;
        workshopGroups.join(constraint.left(), constraint.right());
    }

    return workshopGroups.groups();
}

vector<Constraint>
Constraints::reduce_and_optimize(vector<Constraint> const& constraints, int workshopCount, bool& isInfeasible)
{
    isInfeasible = false;
    vector<Constraint> res;

    for(Constraint c : constraints)
    {
        ConstraintType newType = c.type();
        bool switchSides = true;
        bool add = true;

        switch(c.type())
        {
            case SlotContainsWorkshop: newType = WorkshopIsInSlot; break;
            case SlotNotContainsWorkshop: newType = WorkshopIsNotInSlot; break;
            case WorkshopContainsParticipant: newType = ParticipantIsInWorkshop; break;
            case WorkshopNotContainsParticipant: newType = ParticipantIsNotInWorkshop; break;

            case SlotsHaveSameWorkshops:
            {
                // This constraint is always either a tautology or a contradiction.
                //
                add = false;
                if(c.left() != c.right())
                {
                    isInfeasible = true;
                }
                break;
            }

            default:
            {
                switchSides = false;
                break;
            }
        }

        if(add)
        {
            res.push_back(Constraint(newType, switchSides ? c.right() : c.left(), switchSides ? c.left() : c.right(), c.extra()));
        }
    }

    return expand_dependent_constraints(res, workshopCount);
}
