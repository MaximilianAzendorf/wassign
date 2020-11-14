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

#include "Rng.h"

int Rng::next()
{
    _mutex.lock();
    int ret = _dist(_mt);
    _mutex.unlock();

    return ret;
}

int Rng::next(int min, int max)
{
    return min + next() % (max - min);
}

void Rng::seed(int seed)
{
    _mutex.lock();
    _mt = std::mt19937(seed);
    _mutex.unlock();
}

std::mt19937 Rng::engine()
{
    return _mt;
}
