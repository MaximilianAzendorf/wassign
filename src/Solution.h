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
#include "Scheduling.h"
#include "Assignment.h"

class Solution
{
private:
    const_ptr<Scheduling> _scheduling;
    const_ptr<Assignment> _assignment;

public:
    Solution();

    Solution(const_ptr<Scheduling> scheduling, const_ptr<Assignment> assignment);

    [[nodiscard]] const_ptr<Scheduling> const& scheduling() const;

    [[nodiscard]] const_ptr<Assignment> const& assignment() const;

    [[nodiscard]] InputData const& input_data() const;

    [[nodiscard]] bool is_invalid() const;

    [[nodiscard]] static Solution invalid();
};


