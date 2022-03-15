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

#include "ChoiceData.h"

ChoiceData::ChoiceData(string name, int min, int max)
        : name(std::move(name)), min(min), max(max), continuation(std::nullopt)
{
}

ChoiceData::ChoiceData(string name, int min, int max, int continuation, bool isOptional)
        : name(std::move(name)), min(min), max(max), continuation(continuation), isOptional(isOptional)
{
}

ChoiceData::ChoiceData(string name, int min, int max, optional<int> continuation, bool isOptional)
        : name(std::move(name)), min(min), max(max), continuation(continuation), isOptional(isOptional)
{
}

bool ChoiceData::has_continuation() const
{
    return continuation.has_value();
}

int ChoiceData::continuation_value() const
{
    return continuation.value();
}
