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
#include "InputData.h"
#include "Options.h"
#include "Solution.h"
#include "Score.h"

/**
 * Calculates the score of a given solution.
 */
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

    /**
     * Determines if the given solution is valid.
     */
    [[nodiscard]] virtual bool is_feasible(Solution const& solution) const;

    /**
     * Calculates the score of the given solution.
     */
    [[nodiscard]] virtual Score evaluate(Solution const& solution) const;
};


