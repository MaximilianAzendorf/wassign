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

#include "Types.h"
#include "Constraint.h"

/**
 * Contains algorithms operating on a list of constraints.
 */
class Constraints
{
private:
    Constraints() = default;

    /**
     * Returns critical sets that have to be satisfied because else the solution would not satisfy some fixed assignments
     * (specified by ChooserIsInChoice constraints).
     */
    static vector<vector<int>> get_mandatory_critical_sets(vector<Constraint> const& constraints);

    static vector<Constraint> expand_dependent_constraints(vector<Constraint> const& constraints, int choiceCount);

public:
    /**
     * Returns all choices grouped together when they are connected with a ChoicesHaveSameChoosers constraint.
     */
    static vector<vector<int>> get_dependent_choices(vector<Constraint> const& constraints, int choiceCount);

    /**
     * Replaces some constraint types with their dual type (e.g. SlotContainsChoice becomes ChoiceIsInSlot) and removes
     * constraints that are a tautology. If there are constraints that represent a contradiction, isInfeasible will be
     * set to true.
     */
    static vector<Constraint> reduce_and_optimize(vector<Constraint> const& constraints,
                                                  int choiceCount,
                                                  bool& isInfeasible);
};


