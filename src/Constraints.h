#pragma once

#include "Types.h"
#include "Constraint.h"

class Constraints
{
private:
    Constraints() = default;

    static vector<vector<int>> get_mandatory_critical_sets(vector<Constraint> const& constraints);

    static vector<Constraint> expand_dependent_constraints(vector<Constraint> const& constraints, int workshopCount);

public:
    static vector<vector<int>> get_dependent_workshops(vector<Constraint> const& constraints, int workshopCount);

    static vector<Constraint> reduce_and_optimize(vector<Constraint> const& constraints,
                                                  int workshopCount,
                                                  bool& isInfeasible);
};


