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
#include "Options.h"

class Status
{
private:
    Status() = default;

    static string color(int foregroundColor);

    static string color_reset();

    static bool _output;

    static const_ptr<Options> _options;

public:
    static void enable_output(const_ptr<Options> options);

    static void info(string const& text);

    static void info_important(string const& text);

    static void warning(string const& text);

    static void error(string const& text);
};


