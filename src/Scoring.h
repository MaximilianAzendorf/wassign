#pragma once

#include "Types.h"
#include "InputData.h"
#include "Options.h"
#include "Solution.h"
#include "Score.h"

class Scoring
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<Options> _options;
    float _scaling;

    [[nodiscard]] bool satisfies_constraints_scheduling(Solution const& solution) const;

    [[nodiscard]] bool satisfies_constraints_assignment(Solution const& solution) const;

    [[nodiscard]] bool satisfies_constraints(Solution const& solution) const;

    [[nodiscard]] int evaluate_major(Solution const& solution) const;

    [[nodiscard]] float evaluate_minor(Solution const& solution) const;

public:
    Scoring(const_ptr<InputData> inputData, const_ptr<Options> options);

    [[nodiscard]] virtual bool is_feasible(Solution const& solution) const;

    [[nodiscard]] virtual Score evaluate(Solution const& solution) const;
};


