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
#include "ImplicationGraph.h"

#include <ortools/linear_solver/linear_solver.h>

namespace op = operations_research;

/**
 * Contains a single assignment problem represented as a (modified) min-cost-flow problem and serves as the interface
 * for the used MIP solver.
 */
template<typename NodeKey, typename EdgeKey>
class MipFlow
{
private:
    map<NodeKey, int> _nodeMap;
    map<EdgeKey, int> _edgeMap;
    vector<int> _supply;
    vector<vector<int>> _outgoing;
    vector<vector<int>> _incoming;
    vector<int> _edgesMax;
    vector<long> _edgesCost;
    set<int> _blockedEdges;
    vector<int> _solution;
    ImplicationGraph _implGraph;

    bool is_blocked(int edge);

public:
    /**
     * Adds a single node.
     */
    int add_node();

    /**
     * Adds a single node with the given key.
     */
    int add_node(NodeKey key);

    /**
     * Set the (possibly negative) supply of a single node.
     */
    void set_supply(int node, int supply);

    /**
     * Adds a single edge.
     */
    int add_edge(int fromNode, int toNode, int max, long unitCost);

    /**
     * Adds a single edge with the given key.
     */
    int add_edge(EdgeKey key, int fromNode, int toNode, int max, long unitCost);

    /**
     * Blocks the given edge so that it has value 0 in the solution.
     */
    void block_edge(int edge);

    /**
     * Adds an implication to the implication graph.
     */
    void add_implication(int fromEdge, int toEdge);

    /**
     * Adds implications to the implication graph so that all edges given will be equal.
     */
    template<typename EdgeKeyIterator>
    void make_edges_equal(EdgeKeyIterator begin, EdgeKeyIterator end);

    /**
     * Solves this instance with the given MIP solver instance.
     */
    bool solve(op::MPSolver& solver);

    /**
     * Returns the flow of the given edge.
     */
    [[nodiscard]] int solution_value_at(EdgeKey key) const;

    /**
     * Returns the mapping between node keys and node indexes.
     */
    [[nodiscard]] map<NodeKey, int> const& nodes() const;

    /**
     * Returns the mapping between edge keys and edge indexes.
     */
    [[nodiscard]] map<EdgeKey, int> const& edges() const;

    /**
     * Returns the number of nodes in this instance.
     */
    [[nodiscard]] int node_count() const;

    /**
     * Returns the number of edges in this instance.
     */
    [[nodiscard]] int edge_count() const;
};

#include "MipFlow.ipp"