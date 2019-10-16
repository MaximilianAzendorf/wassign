using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace WSolve
{
    public class MultiLevelGaSystem
    {
        public IParameter<int> GenerationSize { get; set; }

        public ISelection Selection { get; set; }
        
        public IFitness Fitness { get; set; }

        public ICrossover Crossover { get; set; }

        public IParameter<int> PopulationSize { get; set; }
        public IParameter<float> CrossoverChance { get; set; }
        public IParameter<float> MutationChance { get; set; }

        public TimeSpan Timeout { get; set; }
        
        public MutationCollection Mutations { get; } = new MutationCollection();

        public (float major, float minor) BestFitness
        {
            get
            {
                var started = _levels.Where(l => l.HasStarted);
                if (!started.Any()) return default;
                else
                {
                    return _levels.Where(l => l.HasStarted).Min(l => l.BestFitness);
                }
            }
        }

        private readonly int[] _preferenceLevels;

        public int NumberOfGeneticThreads => _preferenceLevels.Length;

        public int NumberofSubthreads { get; } = Environment.ProcessorCount / 2;
        public int NumberOfPresolverThreads { get; } = Environment.ProcessorCount / 2;
        public int NumberOfPrefPumpThreads { get; } = 1;
        public InputData InputData { get; }
        public int BucketLimit { get; }

        public int FirstRunningLevel => _levels.OrderBy(l => l.PreferenceLevel).FirstOrDefault(l => l.HasStarted)?.PreferenceLevel ?? 0;

        public int LevelIndex(int preferenceLevel) => Array.IndexOf(_preferenceLevels, preferenceLevel);
        
        public bool PresolversWaiting => _buckets.Any(b => b.Count >= BucketLimit * 19 / 20);

        public int GenerationsPassed => _levels.Sum(l => l?.GenerationsPassed ?? 0);

        public DateTime TimeStarted { get; private set; }
        
        public double Progress => (DateTime.Now - TimeStarted) / Timeout;

        private readonly BlockingCollection<Chromosome>[] _buckets;
        private GaLevel[] _levels;
        private List<Thread> _threads;
        private readonly CancellationTokenSource _cts = new CancellationTokenSource();
        
        public MultiLevelGaSystem(InputData inputData, int bucketLimit)
        {
            BucketLimit = bucketLimit;   
            
            InputData = inputData;

            _preferenceLevels = inputData.Participants.SelectMany(p => p.preferences).Distinct().OrderBy(x => x).ToArray();
             
            _buckets = new BlockingCollection<Chromosome>[InputData.MaxPreference+1];
            for (int i = 0; i < _buckets.Length; i++)
            {
                _buckets[i] = new BlockingCollection<Chromosome>(BucketLimit);
            }
        }

        public void Start()
        {
            if (_threads != null) throw new InvalidOperationException("MLGA already started.");

            TimeStarted = DateTime.Now;
            
            Status.Info($"Starting GA System with {NumberOfPresolverThreads} presolver worker(s), {NumberOfPrefPumpThreads} preference pump worker(s) and {NumberOfGeneticThreads} genetic workers.");

            //Status.Info("Preference levels: " + string.Join(", ", _preferenceLevels));
            
            _threads = new List<Thread>();

            _levels = new GaLevel[_preferenceLevels.Length];
            
            for (int i = 0; i < NumberOfPresolverThreads; i++)
            {
                _threads.Add(new Thread(() => PresolverWorker(_cts.Token)));
                _threads.Last().Start();
            }

            foreach(int i in _preferenceLevels)
            {
                int closurei = i;
                _threads.Add(new Thread(() => GaWorker(_cts.Token, closurei, NumberOfGeneticThreads / Environment.ProcessorCount)));
                _threads.Last().Start();
            }

            for (int i = 0; i < NumberOfPrefPumpThreads; i++)
            {
                _threads.Add(new Thread(() => PrefPumpWorker(_cts.Token)));
                _threads.Last().Start();
            }
        }
        
        public Solution WaitForSolution(TimeSpan outputFrequency)
        {
            while (_levels.All(l => !l.HasFinished))
            {
                Thread.Sleep(outputFrequency.Milliseconds);

                if (_levels.All(l => !l.HasStarted)) continue;
                
                StringBuilder state = new StringBuilder();

                string extraPresolverState = PresolversWaiting ? "   " : "(+)";

                state.Append($"{extraPresolverState} BEST=({((int)BestFitness.major + ",").PadRight(3)} {BestFitness.minor:0.00000}), ETA={((TimeStarted + Timeout) - DateTime.Now).WithoutMilliseconds()}  :: ");
                
                int minStarted = LevelIndex(_levels.Where(l => l.HasStarted).Min(l => l.PreferenceLevel));
                minStarted = Math.Min(_levels.Length - 3, Math.Max(0, minStarted));
                
                for(int i = minStarted; i <= minStarted + 2; i++)
                {
                    GaLevel level = _levels[i];
                    if (level == null) continue;
                    string status = $" [{_levels[i].PreferenceLevel}({_levels[i].GenerationsPassed}):{level.CurrentGeneration?.Count ?? 0}/{_buckets[i].Count}] ";
                    state.Append(status);
                }

                Status.Info(state.ToString());
            }
            
            Status.Info("Stopped GA System. Waiting for workers to finish.");
            _cts.Cancel();
            foreach (var t in _threads) t?.Join();

            var best = _levels.Select(l => l.BestChromosome).OrderBy(Fitness.Evaluate).First();
            
            best = LocalOptimization.Apply(best, Fitness, out int altCount);
            
            Status.Info($"Local Optimizations made {altCount} alteration(s).");
            Status.Info("Final Fitness: " + Fitness.Evaluate(best));
            
            return new Solution(InputData,
                Enumerable.Range(0, InputData.Workshops.Count).Select(w => (w, best.Slot(w))),
                Enumerable.Range(0, InputData.Participants.Count).SelectMany(p =>
                    Enumerable.Range(0, InputData.Slots.Count).Select(s => (p, best.Workshop(p,s)))));
        }

        public void AddToBucket(Chromosome chromosome, int prefLevel)
        {
            _buckets[prefLevel].TryAdd(chromosome);
        }

        private void PresolverWorker(CancellationToken token)
        {
            bool controller(Solution _)
            {
                while (PresolversWaiting && !token.IsCancellationRequested)
                {
                    Thread.Sleep(100);
                }

                return true;
            }
            
            GreedySolver baseSolver = new GreedySolver();
            PrefPumpPresolver presolver = new PrefPumpPresolver();
            
            presolver.Presolve(InputData, 
                baseSolver.SolveIndefinitely(InputData, token)
                    .TakeWhile(controller)
                    .Where(s => Fitness.IsFeasible(Chromosome.FromOutput(InputData, s))), token,
                (r, pref) =>
                {
                    if(Fitness.IsFeasible(r))
                        AddToBucket(r, pref);
                });
        }
        
        private void GaWorker(CancellationToken token, int level, int threads)
        {
            GaLevel ga = new GaLevel(this, level, _buckets[level]);
            
            _levels[LevelIndex(level)] = ga;
            
            ga.Run(token);
        }

        private volatile int _prefPumpMinFound = int.MaxValue;
        private void PrefPumpWorker(CancellationToken ct)
        {
            Thread.Sleep(5000);
            Status.Info("Begin preference pump.");

            while (!ct.IsCancellationRequested)
            {
                _prefPumpMinFound = Math.Min(_prefPumpMinFound, FirstRunningLevel);
                foreach (GaLevel level in _levels.Where(l => l.HasStarted))
                {
                    Chromosome[] chromosomes = level?.CurrentGeneration?.Select(c => c.Copy())
                        ?.Where(Fitness.IsFeasible)?.ToArray();
                    if (chromosomes == null)
                    {
                        Thread.Sleep(100);
                        continue;
                    }

                    foreach (Chromosome c in chromosomes)
                    {
                        if (ct.IsCancellationRequested) return;
                        var r = PrefPumpHeuristic.TryPump(c, level.PreferenceLevel, int.MaxValue,
                            TimeSpan.FromSeconds(10));
                        if (r == PrefPumpResult.Partial || r == PrefPumpResult.Success)
                        {
                            if (r == PrefPumpResult.Success && c.MaxUsedPreference < _prefPumpMinFound)
                            {
                                Status.Info(
                                    $"Preference Pump Heuristic found better preference limit ({c.MaxUsedPreference}).");
                                _prefPumpMinFound = c.MaxUsedPreference;
                            }

                            while (!_buckets[c.MaxUsedPreference].TryAdd(c))
                            {
                                _buckets[c.MaxUsedPreference].Take();
                            }
                        }
                    }
                }
            }
        }
    }
}