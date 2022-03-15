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

template<typename T>
class UnionFind
{
private:
    map<T, int> _elements;
    vector<T> _parents;
    vector<T> _heights;

public:
    explicit UnionFind(T max);

    int join(T element1, T element2);

    [[nodiscard]] int find(T element) const;

    [[nodiscard]] int size() const;

    [[nodiscard]] vector<vector<T>> groups() const;
};

#include "UnionFind.ipp"