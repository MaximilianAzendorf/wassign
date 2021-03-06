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
#include <random>
#include <mutex>

class Rng
{
private:
    inline static std::mutex _mutex;
    inline static unique_ptr<std::mt19937> _mt;
    inline static unique_ptr<std::uniform_int_distribution<int>> _dist;

    Rng() = default;

public:
    static int next();

    static int next(int min, int max);

    static void seed(int seed);

    static std::mt19937& engine();
};


