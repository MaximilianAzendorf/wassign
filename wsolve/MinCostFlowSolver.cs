using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using Google.OrTools.Graph;
using Google.OrTools.LinearSolver;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.Constraints;
using Constraint = WSolve.ExtraConditions.Constraints.Constraint;

namespace WSolve
{
    public class MinCostFlowSolver : SolverBase
    {
        private const int WORKSHOP = -1;
        private const int SLOT = -2;
        
        private class StaticData
        {
            public MipFlow<(int, int), (int, int)> BaseFlow = new MipFlow<(int, int), (int, int)>();
            public HashSet<(int from, int to)> BlockedEdges = new HashSet<(int @from, int to)>();
            public List<Constraint> Constraints = new List<Constraint>();
        }

        private static readonly int ThreadCount = Environment.ProcessorCount;
        private static readonly int NeighborSampleSize = 1;
        
        public const string PARAM_NAME = "mcf";
        
        private volatile int _tries = 0;
        private readonly List<TimeSpan> _solveTime = new List<TimeSpan>();
        
        private Candidate[] _bests;
        private (float major, float minor)[] _bestsFitness;
        private Thread[] _threads;
        
        private CancellationTokenSource _cts = new CancellationTokenSource();
        
        public override Solution Solve(InputData inputData)
        {
            if (Math.Pow(inputData.MaxPreference + 1, Options.PreferenceExponent) * inputData.ParticipantCount >= long.MaxValue)
            {
                Status.Warning("The preference exponent is too large; computations may cause an integer overflow.");
            }
            
            CriticalSetAnalysis csAnalysis = GetCsAnalysis(inputData);
            StaticData staticData = GenerateStaticGraphData(inputData);

            Solution solution;
            
            if (Options.Any || inputData.SlotCount == 1)
            {
                Status.Info("Computing solution.");
                solution = SolveAssignment(inputData, NewSolver(), SchedulingSolver.Solve(inputData), csAnalysis,
                    staticData)?.ToSolution();
            }
            else
            {
                Status.Info($"Started min cost flow solver with {ThreadCount} thread(s).");
                IScore score = new Score(inputData);

                _bests = new Candidate[ThreadCount];
                _bestsFitness = new (float major, float minor)[ThreadCount];
                _threads = new Thread[ThreadCount];

                Array.Fill(_bestsFitness, (float.PositiveInfinity, float.PositiveInfinity));

                ConcurrentDictionary<Scheduling, Candidate> doneSchedulings =
                    new ConcurrentDictionary<Scheduling, Candidate>();

                for (int i = 0; i < _threads.Length; i++)
                {
                    int tid = i;
                    _threads[i] = new Thread(() =>
                        DoShotgunHillClimbing(tid, inputData, csAnalysis, staticData, score, doneSchedulings,
                            _cts.Token));
                    _threads[i].Start();
                }

                DateTime startTime = DateTime.Now;
                TimeSpan timeout = TimeSpan.FromSeconds(Options.TimeoutSeconds);
                var lastFitness = (float.PositiveInfinity, float.PositiveInfinity);

                while (DateTime.Now < startTime + timeout)
                {
                    Thread.Sleep(1000);

                    var bestFitness = _bestsFitness.Min();
                    //if (!float.IsFinite(bestFitness.major)) continue;
                    string newString = bestFitness != lastFitness ? "NEW" : "   ";
                    lastFitness = bestFitness;
                    lock (_solveTime)
                    {
                        if (!_solveTime.Any())
                        {
                            _solveTime.Add(TimeSpan.Zero);
                        }

                        Status.Info(
                            $"{newString} BEST=({(bestFitness.major + ",")} {bestFitness.minor:0.00000}), ETA={(startTime + timeout - DateTime.Now).ToStringNoMilliseconds()}, TRIES={_tries} ({doneSchedulings.Count}), STIME={_solveTime.Min().TotalSeconds:0.000}s-{_solveTime.Max().TotalSeconds:0.000}s (m={_solveTime.Median().TotalSeconds:0.000}s)");
                        _solveTime.Clear();
                    }
                }

                Status.Info("Stopped min cost flow solver. Waiting for workers to finish.");
                _cts.Cancel();

                foreach (var t in _threads)
                {
                    t.Join();
                }

                solution = _bests
                    .Zip(_bestsFitness, (c, f) => (c, f))
                    .OrderBy(x => x.f)
                    .First().c
                    .ToSolution();
            }

            return solution;
        }
        
        private StaticData GenerateStaticGraphData(InputData inputData)
        {
            StaticData sgd = new StaticData();
            
            for (int p = 0; p < inputData.ParticipantCount; p++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    sgd.BaseFlow.AddNode((p, s));
                }
            }

            for (int w = 0; w < inputData.WorkshopCount; w++)
            {
                sgd.BaseFlow.AddNode((WORKSHOP, w));
            }

            for (int s = 0; s < inputData.SlotCount; s++)
            {
                sgd.BaseFlow.AddNode((SLOT, s));
            }

            sgd.Constraints = inputData.AssignmentConstraints.ToList();
            
            return sgd;
        }
        
        private HashSet<(int start, int end)> GetBlockedConstraintEdges(InputData inputData, Scheduling scheduling, StaticData sgd)
        {
            HashSet<(int start, int end)> blockedEdges = new HashSet<(int start, int end)>();

            void fixAssignment(int p, int w)
            {
                int s = scheduling[w];
                int from = sgd.BaseFlow.Nodes[(p, s)];
                
                for (int wBlocked = 0; wBlocked < inputData.WorkshopCount; wBlocked++)
                {
                    if(w == wBlocked) continue;
                    if(scheduling[wBlocked] != s) continue;
                    
                    int to = sgd.BaseFlow.Nodes[(WORKSHOP, wBlocked)];
                    blockedEdges.Add((from, to));
                }
            }

            void preventAssignment(int p, int w)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    int from = sgd.BaseFlow.Nodes[(p, s)];
                    int to = sgd.BaseFlow.Nodes[(WORKSHOP, w)];
                    blockedEdges.Add((from, to));
                }
            }
            
            foreach (var constraint in sgd.Constraints)
            {
                switch (constraint)
                {
                    case ContainsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        fixAssignment(c.Owner.Id, c.Element.Id);
                        break;
                    }
                    case ContainsConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        fixAssignment(c.Element.Id, c.Owner.Id);
                        break;
                    }
                    case ContainsNotConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        preventAssignment(c.Owner.Id, c.Element.Id);
                        break;
                    }
                    case ContainsNotConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        preventAssignment(c.Element.Id, c.Owner.Id);
                        break;
                    }
                    case SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless> _:
                    {
                        // This is handled elsewhere.
                        break;
                    }
                    default:
                    {
                        throw new InvalidOperationException("This kind of constraint is not compatible with the min cost flow solver.");
                    }
                }
            }

            return blockedEdges;
        }

        private Solver NewSolver(int tid = 0) => Solver.CreateSolver("Solver" + tid, "CBC_MIXED_INTEGER_PROGRAMMING");

        private void DoShotgunHillClimbing(int tid, InputData inputData, CriticalSetAnalysis csAnalysis, StaticData sgd, 
            IScore score, ConcurrentDictionary<Scheduling, Candidate> doneSchedulings, CancellationToken ctoken)
        {
            using Solver solver = NewSolver(tid);
            
            using IEnumerator<Scheduling> primalSolutions = SchedulingSolver
                .SolveIndefinitely(inputData, csAnalysis, ctoken)
                .GetEnumerator();
            
            while (primalSolutions.MoveNext() && !ctoken.IsCancellationRequested)
            {
                _tries++;

                Scheduling scheduling = primalSolutions.Current;
                Candidate? localBestSolution = null;

                var localBestFitness = (float.PositiveInfinity, float.PositiveInfinity);
            
                while (true)
                {
                    bool foundNeighbor = false;
                    foreach (var n in 
                        localBestSolution == null 
                            ? new[] {scheduling} 
                            : FeasibleNeighbors(inputData, scheduling)
                                .Take(NeighborSampleSize))
                    {
                        if (ctoken.IsCancellationRequested)
                        {
                            return;
                        }
                        
                        if (!doneSchedulings.TryGetValue(n, out var c))
                        {
                            var newc = SolveAssignment(inputData, solver, scheduling, csAnalysis, sgd);
                            if (newc == null)
                            {
                                continue;
                            }
                            
                            c = newc.Value;
                            
                            doneSchedulings.AddOrUpdate(n, c, (unused0, unused1) => c);
                        }

                        (float major, float minor) f = score.Evaluate(c);

                        if (f.CompareTo(localBestFitness) < 0)
                        {
                            foundNeighbor = true;
                            localBestSolution = c;
                            localBestFitness = f;
                            scheduling = n;
                            
                            if (f.CompareTo(_bestsFitness[tid]) < 0)
                            {
                                _bests[tid] = localBestSolution.Value;
                                _bestsFitness[tid] = f;
                            }
                        }
                    }

                    if (!foundNeighbor)
                    {
                        break;
                    }
                }
            }
        }

        private IEnumerable<Scheduling> FeasibleNeighbors(InputData inputData, Scheduling scheduling)
        {
            scheduling = new Scheduling(scheduling);

            foreach (int w in Enumerable.Range(0, inputData.WorkshopCount).OrderBy(_ => RNG.NextInt()))
            {
                foreach (int s in Enumerable.Range(0, inputData.SlotCount).OrderBy(_ => RNG.NextInt()))
                {
                    if(scheduling[w] == s) continue;
                    
                    int origs = scheduling[w];
                    scheduling[w] = s;

                    if (scheduling.IsFeasible())
                    {
                        yield return scheduling;
                    }
                    
                    scheduling[w] = origs;
                }
            }
        }

        private Candidate? SolveAssignment(InputData inputData, Solver solver, Scheduling scheduling, CriticalSetAnalysis csAnalysis, StaticData sgd)
        {
            int prefIdx = inputData.PreferenceLevels.FindIndex(x => x == csAnalysis.PreferenceBound);
            int minIdx = prefIdx;
            int maxIdx = inputData.PreferenceLevels.Count;
            Solution bestSol = null;
            Solution sol;

            do
            {
                int prefLimit = inputData.PreferenceLevels[prefIdx];
                sol = SolveAssignment(inputData, solver, scheduling, sgd, prefLimit);
                if (sol == null)
                {
                    minIdx = prefIdx + 1;
                }
                else
                {
                    bestSol = sol;
                    maxIdx = prefIdx - 1;
                }

                prefIdx = (maxIdx + minIdx) / 2;
            } while (maxIdx > minIdx);

            if (bestSol == null)
            {
                return null;
            }
            
            return Candidate.FromSolution(inputData, bestSol);
        }
        
        private Solution SolveAssignment(InputData inputData, Solver solver, Scheduling scheduling, StaticData sgd, int preferenceLimit)
        {
            if (scheduling == null)
            {
                return null;
            }
            
            var flow = sgd.BaseFlow.Fork();
            
            for (int p = 0; p < inputData.ParticipantCount; p++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    flow.AddSupply(flow.Nodes[(p, s)], 1);
                }
            }

            for (int w = 0; w < inputData.WorkshopCount; w++)
            {
                flow.AddSupply(flow.Nodes[(WORKSHOP, w)], -inputData.Workshops[w].min);
            }

            for (int s = 0; s < inputData.SlotCount; s++)
            {
                // Count the number of participants that will already be absorbed by the workshop nodes.
                //
                int coveredParticipants = scheduling.AsEnumerable()
                    .Where(x => x.slot == s)
                    .Sum(x => inputData.Workshops[x.workshop].min);
                
                flow.AddSupply(flow.Nodes[(SLOT, s)], -(inputData.ParticipantCount - coveredParticipants));
            }
            
            // Then, generate all edges
            //
            var edges = new SortedSet<(int start, int end, int cap, long cost)>();

            for (int p = 0; p < inputData.ParticipantCount; p++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    for (int w = 0; w < inputData.WorkshopCount; w++)
                    {
                        if (scheduling[w] != s) continue;

                        if(inputData.Participants[p].preferences[w] > preferenceLimit) continue;
                        
                        checked
                        {
                            int start = flow.Nodes[(p, s)];
                            int end = flow.Nodes[(WORKSHOP, w)];
                            long cost = (long) Math.Pow(inputData.Participants[p].preferences[w] + 1,
                                Options.PreferenceExponent);
                            
                            edges.Add((start, end, 1, cost));
                        }
                    }
                }
            }

            for (int w = 0; w < inputData.WorkshopCount; w++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    if(scheduling[w] != s) continue;

                    int start = flow.Nodes[(WORKSHOP, w)];
                    int end = flow.Nodes[(SLOT, s)];
                    int cap = inputData.Workshops[w].max - inputData.Workshops[w].min;

                    edges.Add((start, end, cap, 0));
                }
            }
            
            // Remove all blocked edges
            //
            var blockedEdges = GetBlockedConstraintEdges(inputData, scheduling, sgd);
            blockedEdges.UnionWith(sgd.BlockedEdges);
            
            foreach (var edge in edges.ToList())
            {
                if (blockedEdges.Contains((edge.start, edge.end)))
                {
                    edges.Remove(edge);
                }
            }
            
            foreach (var edge in edges)
            {
                flow.AddEdge((edge.start, edge.end), edge.start, edge.end, edge.cap, edge.cost);
            }
            
            // Create edge groups
            //
            foreach (var group in Constraint.GetDependentWorkshops(inputData.AssignmentConstraints, inputData)
                .Where(g => g.Length > 1))
            {
                for (int p = 0; p < inputData.ParticipantCount; p++)
                {
                    List<(int, int)> edgeGroup = new List<(int, int)>();
                    foreach (var w in group)
                    {
                        int s = scheduling[w];

                        int from = flow.Nodes[(p, s)];
                        int to = flow.Nodes[(WORKSHOP, w)];

                        edgeGroup.Add((from, to));
                    }

                    flow.CreateEdgeGroupConditional(edgeGroup);
                }
            }

            // ... and solve this instance.
            //
            Stopwatch sw = Stopwatch.StartNew();
            var solverStatus = flow.Solve(solver);
            sw.Stop();

            lock (_solveTime)
            {
                _solveTime.Add(sw.Elapsed);
            }
            
            if (!solverStatus)
            {
                return null;
            }
            
            // Now we have to extract the assignment solution from the min cost flow solution
            //
            var assignment = new List<(int p, int w)>();
            for (int p = 0; p < inputData.ParticipantCount; p++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    for (int w = 0; w < inputData.WorkshopCount; w++)
                    {
                        int from = flow.Nodes[(p, s)];
                        int to = flow.Nodes[(WORKSHOP, w)];
                        
                        if (flow.Edges.ContainsKey((from, to)) && flow.SolutionValue((from, to)) == 1)
                        {
                            assignment.Add((p, w));
                        }
                    }
                }
            }

            return new Solution(inputData, scheduling, assignment);
        }
    }
}