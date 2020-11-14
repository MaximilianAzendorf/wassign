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

#include "Score.h"

#include "Util.h"
#include "InputData.h"
#include "Options.h"
#include "Solution.h"

string Score::to_str() const
{
    return "(" + str(major, 0) + ", " + str(minor, 5) + ")";
}

bool Score::is_finite() const
{
    return std::isfinite(minor) && std::isfinite(major);
}


bool Score::operator<(Score const& other) const
{
    if(major == other.major)
    {
        return minor < other.minor;
    }
    else
    {
        return major < other.major;
    }
}
bool Score::operator==(Score const& other) const
{
    return major == other.major && minor == other.minor;
}

bool Score::operator!=(Score const& other) const
{
    return !(*this == other);
}
