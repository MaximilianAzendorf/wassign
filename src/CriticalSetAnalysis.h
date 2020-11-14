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

class CriticalSetAnalysis
{
private:
    vector<CriticalSet> _sets;
    const_ptr<InputData> _inputData;
    int preferenceBound;

    void analyze();

public:
    inline static const seconds ProgressInterval = seconds(3);

    CriticalSetAnalysis(const_ptr<InputData> inputData, bool analyze = true);

    [[nodiscard]] vector<CriticalSet> for_preference(int preference) const;

    [[nodiscard]] vector<CriticalSet> const& sets() const;

    [[nodiscard]] int preference_bound() const;

    [[nodiscard]] static CriticalSetAnalysis empty(const_ptr<InputData> inputData);
};


