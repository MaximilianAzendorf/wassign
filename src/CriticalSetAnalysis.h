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
#include "CriticalSet.h"
#include "InputData.h"

/**
 * Critical set analysis is used as a heuristic by the scheduling solver to produce schedulings that are more likely
 * to produce a better solution. For more details see the documentation.
 */
class CriticalSetAnalysis
{
private:
    vector<CriticalSet> _sets;
    const_ptr<InputData> _inputData;
    int preferenceBound;

    /**
     * Performs the analysis.
     */
    void analyze(bool simplify);

public:
    /**
     * After this amount of time, progress updates will be printed to the output while analyzing.
     */
    inline static const seconds ProgressInterval = seconds(3);

    /**
     * Do not print progress updates ever if this is set to true.
     */
    bool quiet = false;

    /**
     * Constructor.
     * @param inputData The input data to analyze.
     * @param analyze If this is set to false, no analysis will be performed and this instance will only hold dummy data.
     */
    explicit CriticalSetAnalysis(const_ptr<InputData> inputData, bool analyze = true, bool simplify = true);

    /**
     * Returns all critical sets relevant for the given preference bound.
     */
    [[nodiscard]] vector<CriticalSet> for_preference(int preference) const;

    /**
     * Returns all critical sets.
     */
    [[nodiscard]] vector<CriticalSet> const& sets() const;

    /**
     * Returns an upper bound for the lowest preference a solution can have.
     */
    [[nodiscard]] int preference_bound() const;

    /**
     * Calls the constructor with analyze = false.
     */
    [[nodiscard]] static CriticalSetAnalysis empty(const_ptr<InputData> inputData);
};


