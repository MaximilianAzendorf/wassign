#pragma once

#include "MipFlow.h"

#include "Util.h"

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::add_node()
{
    _solution.clear();
    _outgoing.push_back({});
    _incoming.push_back({});
    _supply.push_back(0);
    return node_count() - 1;
}

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::add_node(NodeKey key)
{
    int node = add_node();
    _nodeMap[key] = node;
    return node;
}

template<typename NodeKey, typename EdgeKey>
void MipFlow<NodeKey, EdgeKey>::add_supply(int node, int supply)
{
    _solution.clear();
    _supply[node] = supply;
}

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::add_edge(int fromNode, int toNode, int max, long unitCost)
{
    _solution.clear();
    _edgesMax.push_back(max);
    _edgesCost.push_back(unitCost);
    int edge = edge_count() - 1;

    _outgoing[fromNode].push_back(edge);
    _incoming[toNode].push_back(edge);

    return edge;
}

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::add_edge(EdgeKey key, int fromNode, int toNode, int max, long unitCost)
{
    int edge = add_edge(fromNode, toNode, max, unitCost);
    _edgeMap[key] = edge;
    return edge;
}

template<typename NodeKey, typename EdgeKey>
template<typename EdgeKeyIterator>
void MipFlow<NodeKey, EdgeKey>::create_edge_group_or_block_edges(EdgeKeyIterator begin, EdgeKeyIterator end)
{
    vector<int> edges;
    bool blocked = false;

    for(; begin != end; begin++)
    {
        auto edgeIt = _edgeMap.find(*begin);

        if(edgeIt == _edgeMap.end())
        {
            blocked = true;
        }
        else
        {
            edges.push_back(edgeIt->second);
        }
    }

    if(blocked)
    {
        for(int edge : edges)
        {
            _blockedEdges.push_back(edge);
        }
    }
    else
    {
        _edgeGroups.push_back(edges);
    }
}

template<typename NodeKey, typename EdgeKey>
bool MipFlow<NodeKey, EdgeKey>::solve(op::MPSolver& solver)
{
    solver.Clear();
    vector<op::MPVariable*> edgeVariables(edge_count());
    op::MPObjective* minTerm = solver.MutableObjective();

    for(int i = 0; i < edge_count(); i++)
    {
        edgeVariables[i] = solver.MakeNumVar(0, _edgesMax[i], "v" + str(i));

        minTerm->SetCoefficient(edgeVariables[i], _edgesCost[i]);
    }

    for(int i = 0; i < _edgeGroups.size(); i++)
    {
        op::MPVariable* switchVar = solver.MakeBoolVar("s" + str(i));

        for(int e : _edgeGroups[i])
        {
            op::MPConstraint* eqConst = solver.MakeRowConstraint(0, 0);
            eqConst->SetCoefficient(edgeVariables[e], 1);
            eqConst->SetCoefficient(switchVar, -1);
        }
    }

    for(int edge : _blockedEdges)
    {
        op::MPConstraint* zeroConst = solver.MakeRowConstraint(0, 0);
        zeroConst->SetCoefficient(edgeVariables[edge], 1);
    }

    for(int i = 0; i < node_count(); i++)
    {
        op::MPConstraint* nodeConst = solver.MakeRowConstraint(-_supply[i], -_supply[i]);
        for(int inEdge : _incoming[i])
        {
            nodeConst->SetCoefficient(edgeVariables[inEdge], 1);
        }

        for(int outEdge : _outgoing[i])
        {
            nodeConst->SetCoefficient(edgeVariables[outEdge], -1);
        }
    }

    minTerm->SetMinimization();

    //solver.EnableOutput();
    bool success = solver.Solve() == op::MPSolver::OPTIMAL;

    if(success)
    {
        _solution.clear();
        _solution.resize(edgeVariables.size());

        for(int i = 0; i < edgeVariables.size(); i++)
        {
            _solution[i] = edgeVariables[i]->solution_value();
        }
    }

    return success;
}

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::solution_value_at(EdgeKey key) const
{
    if(_solution.empty())
    {
        throw std::logic_error("The MIP flow instance is not solved.");
    }

    if(_edgeMap.find(key) == _edgeMap.end())
    {
        return 0;
    }

    return _solution[_edgeMap.at(key)];
}

template<typename NodeKey, typename EdgeKey>
map<NodeKey, int> const& MipFlow<NodeKey, EdgeKey>::nodes() const
{
    return _nodeMap;
}

template<typename NodeKey, typename EdgeKey>
map<EdgeKey, int> const& MipFlow<NodeKey, EdgeKey>::edges() const
{
    return _edgeMap;
}

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::node_count() const
{
    return _outgoing.size();
}

template<typename NodeKey, typename EdgeKey>
int MipFlow<NodeKey, EdgeKey>::edge_count() const
{
    return _edgesMax.size();
}
