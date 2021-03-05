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

#include "Solution.h"
#include <cassert>

Solution::Solution()
        : _scheduling(nullptr), _assignment(nullptr)
{
}

Solution::Solution(const_ptr<Scheduling> scheduling, const_ptr<Assignment> assignment)
        : _scheduling(std::move(scheduling)), _assignment(std::move(assignment))
{
    // HACK: This somehow breaks wasm builds; just ignore for now.
#ifndef __EMSCRIPTEN__
    assert(&_scheduling->input_data() == &_assignment->input_data());
#endif
}

const_ptr<Scheduling> const& Solution::scheduling() const
{
    return _scheduling;
}

const_ptr<Assignment> const& Solution::assignment() const
{
    return _assignment;
}

InputData const& Solution::input_data() const
{
    return _scheduling->input_data();
}

bool Solution::is_invalid() const
{
    return !_scheduling || !_assignment;
}

Solution Solution::invalid()
{
    return Solution();
}
