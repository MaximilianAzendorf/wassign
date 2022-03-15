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
 * A critical set consists of a set S of choices and a preference P, such that in each solution with a maximum
 * preference of P, each slot must contain at least one choice in S.
 */
class CriticalSet
{
private:
    ordered_set<int> _data;
    int _preference;

public:
    /**
     * Constructor
     */
    CriticalSet(int preference, vector<int> const& data);

    /**
     * Determines if this set is covered by the other one, meaning that the fulfillment of the other set implies the
     * fulfillment of this one.
     */
    [[nodiscard]] bool is_covered_by(CriticalSet const &other) const;

    /**
     * Determines if this set is a superset of the other set.
     */
    [[nodiscard]] bool is_superset_of(CriticalSet const &other) const;

    /**
     * Determines if this set contains the specified item.
     */
    [[nodiscard]] bool contains(int item) const;

    /**
     * Returns the size of this set.
     */
    [[nodiscard]] int size() const;

    /**
     * Returns the preference of this set.
     */
    [[nodiscard]] int preference() const;

    /**
     * Returns the elements of the set.
     */
    [[nodiscard]] ordered_set<int> const& elements() const;
};
