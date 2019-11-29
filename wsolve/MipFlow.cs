using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using Google.OrTools.LinearSolver;

namespace WSolve
{
    public class MipFlow<TNodeKey, TEdgeKey>
    {
        private readonly Dictionary<TNodeKey, int> _nodeMap = new Dictionary<TNodeKey, int>();
        private readonly Dictionary<TEdgeKey, int> _edgeMap = new Dictionary<TEdgeKey, int>();
        private readonly List<int> _supply = new List<int>();
        private readonly List<List<int>> _outgoing = new List<List<int>>();
        private readonly List<List<int>> _incoming = new List<List<int>>();
        private readonly List<(int max, long cost)> _edges = new List<(int max, long cost)>();
        private readonly List<int[]> _edgeGroups = new List<int[]>();
        private readonly List<int> _blockedEdges = new List<int>();
        private int[] _solution;
        
        public IReadOnlyDictionary<TNodeKey, int> Nodes => _nodeMap;
        public IReadOnlyDictionary<TEdgeKey, int> Edges => _edgeMap;

        public int NodeCount => _outgoing.Count;
        public int EdgeCount => _edges.Count;

        public MipFlow()
        {
        }

        private MipFlow(MipFlow<TNodeKey, TEdgeKey> fork)
        {
            _nodeMap = new Dictionary<TNodeKey, int>(fork._nodeMap);
            _edgeMap = new Dictionary<TEdgeKey, int>(fork._edgeMap);
            _supply = fork._supply.ToList();
            _outgoing = new List<List<int>>(fork._outgoing.Select(l => l.ToList()));
            _incoming = new List<List<int>>(fork._incoming.Select(l => l.ToList()));
            _edges = fork._edges.ToList();
            _edgeGroups = fork._edgeGroups.Select(a => a.ToArray()).ToList();
            _blockedEdges = fork._blockedEdges.ToList();
            _solution = fork._solution?.ToArray();
        }
        
        public int AddNode()
        {
            _solution = null;
            _outgoing.Add(new List<int>());
            _incoming.Add(new List<int>());
            _supply.Add(0);
            return NodeCount - 1;
        }

        public int AddNode(TNodeKey key)
        {
            int node = AddNode();
            _nodeMap.Add(key, node);
            return node;
        }

        public void AddSupply(int node, int supply)
        {
            _solution = null;
            _supply[node] = supply;
        }
        
        public int AddEdge(int fromNode, int toNode, int max, long unitCost)
        {
            _solution = null;
            _edges.Add((max, unitCost));
            int edge = EdgeCount - 1;

            _outgoing[fromNode].Add(edge);
            _incoming[toNode].Add(edge);

            return edge;
        }
        
        public int AddEdge(TEdgeKey key, int fromNode, int toNode, int max, long unitCost, bool allOrNothing = false)
        {
            int edge = AddEdge(fromNode, toNode, max, unitCost);
            _edgeMap.Add(key, edge);
            return edge;
        }

        public void CreateEdgeGroup(IEnumerable<TEdgeKey> edges)
        {
            _edgeGroups.Add(edges.Select(e => Edges[e]).ToArray());
        }

        public void CreateEdgeGroupConditional(IEnumerable<TEdgeKey> edgeKeys)
        {
            List<int> edges = new List<int>();
            bool blocked = false;

            foreach (var key in edgeKeys)
            {
                if (!Edges.TryGetValue(key, out var edge))
                {
                    blocked = true;
                }
                else
                {
                    edges.Add(edge);
                }
            }

            if (blocked)
            {
                _blockedEdges.AddRange(edges);
            }
            else
            {
                _edgeGroups.Add(edges.ToArray());
            }
        }

        public int SolutionValue(TEdgeKey key) => _solution?[_edgeMap[key]] 
            ?? throw new InvalidOperationException("The MIP flow instance is not solved.");

        private static readonly object _syncroot = new object();
        
        public bool Solve(Solver solver)
        {
            solver.Clear();
            Variable[] edgeVariables;
            LinearExpr minTerm;

            lock (_syncroot)
            {
                LinearExpr nullExpr = solver.MakeNumVar(0, 0, "_dummy");
                minTerm = 0 * nullExpr;

                edgeVariables = new Variable[EdgeCount];

                for (int i = 0; i < EdgeCount; i++)
                {
                    edgeVariables[i] = solver.MakeNumVar(0, _edges[i].max, "v" + i);

                    minTerm += _edges[i].cost * edgeVariables[i];
                }

                for (int i = 0; i < _edgeGroups.Count; i++)
                {
                    Variable switchVar = solver.MakeBoolVar("s" + i);

                    foreach (var e in _edgeGroups[i])
                    {
                        Debug.Assert(_edges[e].max == 1);
                        solver.Add(edgeVariables[e] - switchVar == 0);
                    }
                }

                foreach (int edge in _blockedEdges)
                {
                    solver.Add(edgeVariables[edge] == 0);
                }

                for (int i = 0; i < NodeCount; i++)
                {
                    LinearExpr sum = nullExpr;
                    sum = _incoming[i].Aggregate(sum, (current, e) => current + edgeVariables[e]);
                    sum = _outgoing[i].Aggregate(sum, (current, e) => current - edgeVariables[e]);

                    solver.Add(sum == -_supply[i]);
                }

            }
            
            solver.Minimize(minTerm);

            bool success = solver.Solve() == Solver.ResultStatus.OPTIMAL;

            if (success)
            {
                _solution = edgeVariables.Select(v => (int) v.SolutionValue()).ToArray();
                Debug.Assert(edgeVariables.All(v => v.SolutionValue() == (int) v.SolutionValue()));
            }
            
            return success;
        }

        public MipFlow<TNodeKey, TEdgeKey> Fork()
        {
            return new MipFlow<TNodeKey, TEdgeKey>(this);
        }
    }
}