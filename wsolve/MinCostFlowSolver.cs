using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Google.OrTools.Graph;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.StatelessAccess;
using WSolve.ExtraConditions.StatelessAccess.Constraints;

namespace WSolve
{
    public class MinCostFlowSolver : SolverBase
    {
        private class StaticData
        {
            public Dictionary<(int p, int s), int> ParticipantNodes = new Dictionary<(int p, int s), int>();
            public Dictionary<int, int> WorkshopNodes = new Dictionary<int, int>();
            public Dictionary<int, int> SlotNodes = new Dictionary<int, int>();
            public HashSet<(int from, int to)> BlockedEdges = new HashSet<(int @from, int to)>();
            public List<Constraint> Constraints = new List<Constraint>();
        }

        private static readonly int ThreadCount = Environment.ProcessorCount;
        private static readonly int NeighborSampleSize = 1;
        
        public const string PARAM_NAME = "mcf";
        
        private volatile int _tries = 0;
        private Chromosome[] _bests;
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
            Func<Chromosome, CustomExtraConditionsBaseStateless> extraConditions = GetExtraConditionsStateless(inputData);
            
            Status.Info($"Started min cost flow solver with {ThreadCount} thread(s).");
            IFitness fitness = new GaSolverFitness(inputData, chromosome => extraConditions?.Invoke(chromosome)?.DirectResult ?? true);
            
            _bests = new Chromosome[ThreadCount];
            _bestsFitness = new (float major, float minor)[ThreadCount];
            _threads = new Thread[ThreadCount];

            Array.Fill(_bestsFitness, (float.PositiveInfinity, float.PositiveInfinity));

            StaticData staticData = GenerateStaticGraphData(inputData, extraConditions);
            ConcurrentSet<Scheduling> doneSchedulings = new ConcurrentSet<Scheduling>();
            
            for (int i = 0; i < _threads.Length; i++)
            {
                int tid = i;
                _threads[i] = new Thread(() => DoShotgunHillClimbing(tid, inputData, csAnalysis, staticData, fitness, doneSchedulings, _cts.Token));
                _threads[i].Start();
            }

            DateTime startTime = DateTime.Now;
            TimeSpan timeout = TimeSpan.FromSeconds(Options.TimeoutSeconds);
            var lastFitness = (float.PositiveInfinity, float.PositiveInfinity);
            
            while (DateTime.Now < startTime + timeout)
            {
                Thread.Sleep(1000);
                
                var bestFitness = _bestsFitness.Min();
                if (!float.IsFinite(bestFitness.major)) continue;
                string newString = bestFitness != lastFitness ? "NEW" : "   ";
                lastFitness = bestFitness;
                
                Status.Info($"{newString} BEST=({((int) bestFitness.major + ",").PadRight(3)} {bestFitness.minor:0.00000}), ETA={(startTime + timeout - DateTime.Now).ToStringNoMilliseconds()}");
            }

            Status.Info("Stopped min cost flow solver. Waiting for workers to finish.");
            _cts.Cancel();

            foreach (var t in _threads)
            {
                t.Join();
            }

            return _bests
                .Zip(_bestsFitness, (c, f) => (c, f))
                .OrderBy(x => x.f)
                .First().c
                .ToSolution();
        }
        
        private StaticData GenerateStaticGraphData(InputData inputData, Func<Chromosome, CustomExtraConditionsBaseStateless> extraConditions)
        {
            StaticData sgd = new StaticData();
            
            // Generate all nodes.
            //
            int nodeIndex = 0;

            for (int p = 0; p < inputData.ParticipantCount; p++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    int newNode = nodeIndex++;
                    sgd.ParticipantNodes[(p, s)] = newNode;
                }
            }

            for (int w = 0; w < inputData.WorkshopCount; w++)
            {
                int newNode = nodeIndex++;
                sgd.WorkshopNodes[w] = newNode;
            }

            for (int s = 0; s < inputData.SlotCount; s++)
            {
                int newNode = nodeIndex++;
                sgd.SlotNodes[s] = newNode;
            }

            if (extraConditions != null)
            {
                sgd.Constraints = GetConstraints(inputData, extraConditions);
            }

            return sgd;
        }

        private List<Constraint> GetConstraints(InputData inputData, Func<Chromosome, CustomExtraConditionsBaseStateless> extraConditions)
        {
            Chromosome chromosome = Chromosome.FromSolution(inputData, new GreedySolver().Solve(inputData));
            return extraConditions(chromosome).StaticConstraints.ToList();
        }
        
        private HashSet<(int start, int end)> GetBlockedConductorEdges(InputData inputData, Scheduling scheduling, StaticData sgd)
        {
            var res = new HashSet<(int start, int end)>();
            
            // Then, we have to block all edges neccessary so that conductors always get into their own workshop.
            //
            for (int w = 0; w < inputData.WorkshopCount; w++)
            {
                int s = scheduling[w];
                foreach (int p in inputData.Workshops[w].conductors)
                {
                    for (int wBlocked = 0; wBlocked < inputData.WorkshopCount; wBlocked++)
                    {
                        if(w == wBlocked) continue;

                        res.Add((sgd.ParticipantNodes[(p, s)],
                            sgd.WorkshopNodes[wBlocked]));
                    }
                }
            }
            
            return res;
        }

        private HashSet<(int start, int end)> GetBlockedConstraintEdges(InputData inputData, Scheduling scheduling, StaticData sgd)
        {
            HashSet<(int start, int end)> blockedEdges = new HashSet<(int start, int end)>();

            void fixAssignment(int p, int w)
            {
                int s = scheduling[w];
                int from = sgd.ParticipantNodes[(p, s)];
                
                for (int wBlocked = 0; wBlocked < inputData.WorkshopCount; wBlocked++)
                {
                    if(w == wBlocked) continue;
                    if(scheduling[wBlocked] != s) continue;
                    
                    int to = sgd.WorkshopNodes[wBlocked];
                    blockedEdges.Add((from, to));
                }
            }

            void preventAssignment(int p, int w)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    int from = sgd.ParticipantNodes[(p, s)];
                    int to = sgd.WorkshopNodes[w];
                    blockedEdges.Add((from, to));
                }
            }
            
            foreach (var constraint in sgd.Constraints)
            {
                switch (constraint)
                {
                    case ContainsConstraint<ParticipantAccessorStateless, WorkshopAccessorStateless> c:
                    {
                        fixAssignment(c.Owner.Id, c.Element.Id);
                        break;
                    }
                    case ContainsConstraint<WorkshopAccessorStateless, ParticipantAccessorStateless> c:
                    {
                        fixAssignment(c.Element.Id, c.Owner.Id);
                        break;
                    }
                    case ContainsNotConstraint<ParticipantAccessorStateless, WorkshopAccessorStateless> c:
                    {
                        preventAssignment(c.Owner.Id, c.Element.Id);
                        break;
                    }
                    case ContainsNotConstraint<WorkshopAccessorStateless, ParticipantAccessorStateless> c:
                    {
                        preventAssignment(c.Element.Id, c.Owner.Id);
                        break;
                    }
                    default:
                    {
                        throw new InvalidOperationException("This kind of constraint is not implemented.");
                    }
                }
            }

            return blockedEdges;
        }

        private void DoShotgunHillClimbing(int tid, InputData inputData, CriticalSetAnalysis csAnalysis, StaticData sgd, IFitness fitness, ConcurrentSet<Scheduling> doneSchedulings, CancellationToken ctoken)
        {
            using IEnumerator<Scheduling> primalSolutions = new GreedySolver()
                .SolveIndefinitelySchedulingOnly(inputData, csAnalysis, ctoken)
                .GetEnumerator();
            
            while (primalSolutions.MoveNext() && !ctoken.IsCancellationRequested)
            {
                if (doneSchedulings.Contains(primalSolutions.Current))
                {
                    continue;
                }

                doneSchedulings.Add(primalSolutions.Current);
                
                var next = HillClimbingIteration(inputData, primalSolutions.Current, csAnalysis, sgd, fitness);

                if (next == null)
                {
                    continue;
                }

                var f = fitness.Evaluate(next.Value);
                if (f.CompareTo(_bestsFitness[tid]) < 0)
                {
                    _bests[tid] = next.Value;
                    _bestsFitness[tid] = f;
                }
            }
        }

        private Chromosome? HillClimbingIteration(InputData inputData, Scheduling scheduling, CriticalSetAnalysis csAnaylsis, StaticData sgd, IFitness fitness)
        {
            Chromosome localBestSolution = SolveAssignment(inputData, scheduling, csAnaylsis, sgd);

            if (!fitness.IsFeasible(localBestSolution))
            {
                return null;
            }

            var localBestFitness = fitness.Evaluate(localBestSolution);
            
            while (true)
            {
                bool foundNeighbor = false;
                foreach (var n in FeasibleNeighbors(inputData, scheduling).Take(NeighborSampleSize))
                {
                    Chromosome c = SolveAssignment(inputData, scheduling, csAnaylsis, sgd);
                    (float major, float minor) f = fitness.Evaluate(localBestSolution);

                    if (f.CompareTo(localBestFitness) < 0)
                    {
                        foundNeighbor = true;
                        localBestSolution = c;
                        localBestFitness = f;
                        scheduling = n;
                    }
                }

                if (!foundNeighbor)
                {
                    break;
                }
            }

            return localBestSolution;
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

        private Chromosome SolveAssignment(InputData inputData, Scheduling scheduling, CriticalSetAnalysis csAnalysis, StaticData sgd)
        {
            int prefLimit = csAnalysis.PreferenceBound;
            Solution sol;

            do
            {
                sol = SolveAssignment(inputData, scheduling, sgd, prefLimit);
                prefLimit = inputData.PreferenceLevels.SkipWhile(p => p <= prefLimit)
                    .Concat(new[] {inputData.MaxPreference}).First();
            } while (sol == null);
            
            return Chromosome.FromSolution(inputData, sol);
        }
        
        private Solution SolveAssignment(InputData inputData, Scheduling scheduling, StaticData sgd, int preferenceLimit)
        {
            Dictionary<int, int> supply = new Dictionary<int, int>();
            
            for (int p = 0; p < inputData.ParticipantCount; p++)
            {
                for (int s = 0; s < inputData.SlotCount; s++)
                {
                    supply[sgd.ParticipantNodes[(p, s)]] = 1;
                }
            }

            for (int w = 0; w < inputData.WorkshopCount; w++)
            {
                supply[sgd.WorkshopNodes[w]] = -inputData.Workshops[w].min;
            }

            for (int s = 0; s < inputData.SlotCount; s++)
            {
                // Count the number of participants that will already be absorbed by the workshop nodes.
                //
                int coveredParticipants = scheduling.AsEnumerable()
                    .Where(x => x.slot == s)
                    .Sum(x => inputData.Workshops[x.workshop].min);
                
                supply[sgd.SlotNodes[s]] = -(inputData.ParticipantCount - coveredParticipants);
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
                            int start = sgd.ParticipantNodes[(p, s)];
                            int end = sgd.WorkshopNodes[w];
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

                    int start = sgd.WorkshopNodes[w];
                    int end = sgd.SlotNodes[s];
                    int cap = inputData.Workshops[w].max - inputData.Workshops[w].min;

                    edges.Add((start, end, cap, 0));
                }
            }
            
            // Remove all blocked edges
            //
            var blockedEdges = GetBlockedConductorEdges(inputData, scheduling, sgd);
            blockedEdges.UnionWith(GetBlockedConstraintEdges(inputData, scheduling, sgd));
            blockedEdges.UnionWith(sgd.BlockedEdges);
            
            foreach (var edge in edges.ToList())
            {
                if (blockedEdges.Contains((edge.start, edge.end)))
                {
                    edges.Remove(edge);
                }
            }

            // Create a MinCostFlow instance out of the nodes and edges ...
            //
            MinCostFlow minCostFlow = new MinCostFlow();
            Dictionary<(int start, int end), int> arcMap = new Dictionary<(int start, int end), int>();

            foreach (var edge in edges)
            {
                int arc = minCostFlow.AddArcWithCapacityAndUnitCost(edge.start, edge.end, edge.cap, edge.cost);
                arcMap.Add((edge.start, edge.end), arc);
            }

            foreach (var kvp in supply)
            {
                minCostFlow.SetNodeSupply(kvp.Key, kvp.Value);
            }
            
            // ... and solve this instance.
            //
            var solverStatus = minCostFlow.Solve();
            if (solverStatus != MinCostFlowBase.Status.OPTIMAL)
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
                        if (!arcMap.TryGetValue((sgd.ParticipantNodes[(p, s)], sgd.WorkshopNodes[w]), out int arc))
                        {
                            continue;
                        }
                        if (minCostFlow.Flow(arc) == 1)
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