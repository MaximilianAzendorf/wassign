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
/**
 * Contains the input data of a choice.
 */
class ChoiceData
{
public:
    /**
     * The name of the choice.
     */
    string name;

    /**
     * The minimum number of choosers this choice can get.
     */
    int min;

    /**
     * The maximum number of choosers this chocie can get.
     */
    int max;

    /**
     * Can contain a choice index that is the next part of this chocie (For multi-part choices).
     */
    optional<int> continuation;

    /**
     * This is true if the choice is optional.
     */
    bool isOptional;

    ChoiceData(string name, int min, int max);

    ChoiceData(string name, int min, int max, int continuation, bool isOptional);

    ChoiceData(string name, int min, int max, optional<int> continuation, bool isOptional);

    [[nodiscard]] bool has_continuation() const;

    [[nodiscard]] int continuation_value() const;
};
