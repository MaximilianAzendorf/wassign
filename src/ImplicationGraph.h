/*
 * Copyright 2021 Maximilian Azendorf
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
 * Models an implication graph. An implication graph is a directed graph between MIP variables where every edge (a,b) is
 * a constraint a <= b. This data structure is then used to calculate a subset of the variables that have to be declared
 * as integer variables.
 */
class ImplicationGraph
{
private:
    struct SCCVertexState
    {
        int index;
        int lowlink;
        bool onStack;
    };

    map<int, set<int>> _adjacency = map<int, set<int>>();

    /**
     * Computes the strongly connected components of the implication graph using Tarjan's algorithm.
     */
    vector<set<int>> compute_sccs();

    /**
     * Implements the recursive part of Tarjan's algorithm for SCC computation.
     */
    void compute_sccs_recursion(int v, int& index, stack<int>& stack,
                                map<int, SCCVertexState>& state, vector<set<int>>& resultVector);

    /**
     * Returns all adjacent variables of v in the given subset.
     */
    vector<int> neighbors_in_subset(int v, set<int> const& varSubset);

    /**
     * Returns a subset of all variables that still may have to be made integer after covering SCCs.
     */
     set<int> get_open_variables(vector<set<int>>& sccs);

     /**
      * Returns a set of variables that dominate the remaining graph after all SCCs were already covered.
      */
      set<int> get_dominating_variables(vector<set<int>>& sccs);

public:
    /**
     * Adds an implication to the implication graph.
     */
    void add_implication(int from, int to);

    /**
     * Returns all implications that are stored in this instance.
     */
    vector<pair<int, int>> get_implications();

    /**
     * Calculates a small set of variables that have to be integer so that all other variables are as well.
     * @return
     */
    set<int> get_integer_variables();
};


