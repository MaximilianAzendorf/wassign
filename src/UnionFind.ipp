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

#include "UnionFind.h"
#include "Util.h"

template<typename T>
UnionFind<T>::UnionFind(T max)
{
    if(max < 0)
    {
        throw std::logic_error("Invalid max value " + str(max) + ".");
    }

    int count = 0;
    for(T i = 0; i < max; i++)
    {
        _elements[i] = count++;
    }

    _parents.resize(count, -1);
    _heights.resize(count, 0);
}

template<typename T>
int UnionFind<T>::find(T element) const
{
    int idx = _elements.at(element);
    while(_parents[idx] != -1)
    {
        idx = _parents[idx];
    }

    return idx;
}

template<typename T>
int UnionFind<T>::size() const
{
    return _elements.size();
}

template<typename T>
int UnionFind<T>::join(T element1, T element2)
{
    int idx1 = find(element1);
    int idx2 = find(element2);

    if(idx1 != idx2)
    {
        if (_heights[idx1] > _heights[idx2])
        {
            std::swap(idx1, idx2);
        }

        _parents[idx1] = idx2;
        _heights[idx2] = std::max(_heights[idx2], _heights[idx1] + 1);
    }

    return idx2;
}

template<typename T>
vector<vector<T>> UnionFind<T>::groups() const
{
    map<int, vector<T>> groupMap;

    for(auto const& element : _elements)
    {
        groupMap[find(element.first)].push_back(element.first);
    }

    vector<vector<int>> res;

    for(auto const& group : groupMap)
    {
        res.push_back(group.second);
    }

    return res;
}
