#pragma once

#include "Types.h"
#include "InputData.h"
#include "Options.h"
#include "Solution.h"
#include "Score.h"

class Scoring
{
private:
    InputData const* _inputData;
    Options const& _options;
    float _scaling;

    [[nodiscard]] bool satisfies_constraints_scheduling(Solution const& solution) const;

    [[nodiscard]] bool satisfies_constraints_assignment(Solution const& solution) const;

    [[nodiscard]] bool satisfies_constraints(Solution const& solution) const;

    [[nodiscard]] int evaluate_major(Solution const& solution) const;

    [[nodiscard]] float evaluate_minor(Solution const& solution) const;

public:
    Scoring(InputData const& inputData, Options const& options);

    [[nodiscard]] bool is_feasible(Solution const& solution) const;

    [[nodiscard]] Score evaluate(Solution const& solution) const;
};


