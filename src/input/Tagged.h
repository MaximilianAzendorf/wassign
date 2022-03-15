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

#include "../Types.h"

enum Tag
{
    Null,
    Ignore,

    Min,
    Max,
    Bounds,
    Parts,
    Optional,
};

class Tagged
{
private:
    Tag _tag;
    vector<int> _values;

    static map<Tag, string> _tagNames;

public:
    Tagged(Tag tag, int value);
    Tagged(Tag tag, vector<int> values);

    [[nodiscard]] Tag tag() const;
    [[nodiscard]] int value(int index = 0) const;
    [[nodiscard]] vector<int> values() const;

    static string tag_name(Tag tag);
};
