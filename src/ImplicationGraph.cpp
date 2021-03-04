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
#include <climits>
#include "ImplicationGraph.h"

vector<set<int>> ImplicationGraph::compute_sccs()
{
    vector<set<int>> sccs;

    int index = 1;
    stack<int> stack;
    map<int, SCCVertexState> state;

    for(auto const& entry : _adjacency)
    {
        compute_sccs_recursion(entry.first, index, stack, state, sccs);
    }

    return sccs;
}

void ImplicationGraph::compute_sccs_recursion(int v, int& index, stack<int>& stack,
                                              map<int, SCCVertexState>& state, vector<set<int>>& resultVector)
{
    SCCVertexState& sv = state[v];
    sv.index = sv.lowlink = index++;
    stack.push(v);
    sv.onStack = true;

    for(int w : _adjacency[v])
    {
        SCCVertexState& sw = state[w];
        if(!sw.index)
        {
            compute_sccs_recursion(w, index, stack, state, resultVector);
            sv.lowlink = std::min(sv.lowlink, sw.lowlink);
        }
        else if(sw.onStack)
        {
            sv.lowlink = std::min(sv.lowlink, sw.index);
        }
    }

    if(sv.lowlink == sv.index)
    {
        set<int> scc;
        int w = 0;
        do
        {
            w = stack.top();
            stack.pop();
            state[w].onStack = false;
            scc.insert(w);
        } while(w != v);

        if(scc.size() > 1)
        {
            // We are only interested in non-trivial SCCs.
            resultVector.push_back(scc);
        }
    }
}

void ImplicationGraph::add_implication(int from, int to)
{
    _adjacency[from].insert(to);
}

vector<pair<int, int>> ImplicationGraph::get_implications()
{
    vector<pair<int, int>> res;
    for(auto const& entry : _adjacency)
    {
        for(auto const& adj : entry.second)
        {
            res.push_back(std::make_pair(entry.first, adj));
        }
    }
    return res;
}

vector<int> ImplicationGraph::neighbors_in_subset(int v, set<int> const& varSubset)
{
    vector<int> neighbors;
    for(int w : _adjacency[v])
    {
        if(varSubset.find(w) != varSubset.end())
        {
            neighbors.push_back(w);
        }
    }

    return neighbors;
}


set<int> ImplicationGraph::get_open_variables(vector<set<int>>& sccs)
{
    set<int> vertices;
    for(auto const& entry : _adjacency) vertices.insert(entry.first);

    for(auto const& scc : sccs)
    {
        for(int v : scc)
        {
            vertices.erase(v);
        }
    }

    for(auto const& entry : _adjacency)
    {
        int v = entry.first;
        if(vertices.find(v) == vertices.end())
        {
            continue;
        }

        if(neighbors_in_subset(v, vertices).empty())
        {
            vertices.erase(v);
        }
    }

    return vertices;
}

set<int> ImplicationGraph::get_dominating_variables(vector<set<int>>& sccs)
{
    set<int> open = get_open_variables(sccs);
    set<int> result;

    while(!open.empty())
    {
        int vBest = -1;
        int vCount = INT_MIN;

        for(int v : open)
        {
            int count = neighbors_in_subset(v, open).size();
            if(vCount < count)
            {
                vBest = v;
                vCount = count;
            }
        }

        open.erase(vBest);
        result.insert(vBest);
        for(int n : neighbors_in_subset(vBest, open))
        {
            open.erase(n);
        }
    }

    return result;
}

set<int> ImplicationGraph::get_integer_variables()
{
    vector<set<int>> sccs = compute_sccs();
    set<int> dominatingSet = get_dominating_variables(sccs);

    for(auto const& scc : sccs)
    {
        dominatingSet.insert(*scc.begin());
    }

    return dominatingSet;
}