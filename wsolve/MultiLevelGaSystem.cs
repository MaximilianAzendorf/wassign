using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace WSolve
{
    public class MultiLevelGaSystem
    {
        private readonly BlockingCollection<Chromosome>[] _buckets;
        private readonly CriticalSetAnalysis _criticalSetAnalysis;
        private readonly CancellationTokenSource _cts = new CancellationTokenSource();
        private readonly int[] _preferenceLevels;
        private GaLevel[] _levels;

        private volatile int _prefPumpMinFound = int.MaxValue;
        private List<Thread> _threads;

        public MultiLevelGaSystem(InputData inputData, CriticalSetAnalysis csAnalysis, int bucketLimit)
        {
            BucketLimit = bucketLimit;

            InputData = inputData;

            _preferenceLevels = inputData.Participants.SelectMany(p => p.preferences).Distinct().OrderBy(x => x)
                .ToArray();

            _buckets = new BlockingCollection<Chromosome>[InputData.MaxPreference + 1];
            for (int i = 0; i < _buckets.Length; i++)
            {
                _buckets[i] = new BlockingCollection<Chromosome>(BucketLimit);
            }

            _criticalSetAnalysis = csAnalysis;
        }

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
                IEnumerable<GaLevel> started = _levels.Where(l => l.HasStarted);
                if (!started.Any())
                {
                    return default;
                }

                return _levels.Where(l => l.HasStarted).Min(l => l.BestFitness);
            }
        }

        public int NumberOfGeneticThreads => _preferenceLevels.Length;

        public int NumberofSubthreads { get; } = Math.Max(1, Environment.ProcessorCount / 2);

        public int NumberOfPresolverThreads { get; } = Math.Max(1, Environment.ProcessorCount / 4);

        public int NumberOfPrefPumpThreads => Options.NoPrefPump ? 0 : 1;

        public InputData InputData { get; }

        public int BucketLimit { get; }

        public int FirstRunningLevel =>
            _levels.OrderBy(l => l.PreferenceLevel).FirstOrDefault(l => l.HasStarted)?.PreferenceLevel ?? 0;

        public bool PresolversWaiting =>
            _buckets[FirstRunningLevel].Count == BucketLimit && Progress > Options.FinalPhaseStart / 2;

        public int GenerationsPassed => _levels.Sum(l => l?.GenerationsPassed ?? 0);

        public DateTime TimeStarted { get; private set; }

        public double Progress => (DateTime.Now - TimeStarted) / Timeout;

        public int LevelIndex(int preferenceLevel)
        {
            return Array.IndexOf(_preferenceLevels, preferenceLevel);
        }

        public void Start()
        {
            if (_threads != null)
            {
                throw new InvalidOperationException("MLGA already started.");
            }

            TimeStarted = DateTime.Now;

            Status.Info(
                $"Starting GA System (Workers: {NumberOfPresolverThreads} Presolver, {(NumberOfPrefPumpThreads == 0 ? "no" : NumberOfPrefPumpThreads.ToString())} PRP, {NumberOfGeneticThreads}*{NumberofSubthreads} GA).");

            _threads = new List<Thread>();

            _levels = new GaLevel[_preferenceLevels.Length];

            for (int i = 0; i < NumberOfPresolverThreads; i++)
            {
                bool tryCriticalSets = i == 0;
                _threads.Add(new Thread(() => PresolverWorker(_cts.Token, tryCriticalSets)));
                _threads.Last().Start();
            }

            foreach (int i in _preferenceLevels)
            {
                int closurei = i;
                _threads.Add(new Thread(() => GaWorker(_cts.Token, closurei)));
                _threads.Last().Start();
            }

            for (int i = 0; i < NumberOfPrefPumpThreads; i++)
            {
                _threads.Add(new Thread(() => PrefPumpWorker(_cts.Token)));
                _threads.Last().Start();
            }
        }

        public Chromosome WaitForSolutionChromosome(TimeSpan outputFrequency)
        {
            while (_levels.All(l => !l.HasFinished))
            {
                Thread.Sleep((int) outputFrequency.TotalMilliseconds);

                if (_levels.All(l => !l.HasStarted))
                {
                    Thread.Sleep((int) outputFrequency.TotalMilliseconds);
                    Status.Info("No GA level started yet; waiting for initial solutions from presolver.");
                    continue;
                }

                var state = new StringBuilder();

                string extraPresolverState = PresolversWaiting ? "   " : "(+)";

                state.Append(
                    $"{extraPresolverState} BEST=({((int) BestFitness.major + ",").PadRight(3)} {BestFitness.minor:0.00000}), ETA={(TimeStarted + Timeout - DateTime.Now).ToStringNoMilliseconds()}  :: ");
                int minStarted = LevelIndex(_levels.Where(l => l.HasStarted).Min(l => l.PreferenceLevel));
                minStarted = Math.Min(_levels.Length - 3, Math.Max(0, minStarted));

                for (int i = minStarted; i <= minStarted + 2; i++)
                {
                    GaLevel level = _levels[i];
                    if (level == null)
                    {
                        continue;
                    }

                    string status =
                        $" [{_levels[i].PreferenceLevel}({_levels[i].GenerationsPassed}):{level.CurrentGeneration?.Count ?? 0}/{_buckets[_levels[i].PreferenceLevel].Count}] ";
                    state.Append(status);
                }

                Status.Info(state.ToString());
            }

            Status.Info("Stopped GA System. Waiting for workers to finish.");
            _cts.Cancel();
            foreach (Thread t in _threads)
            {
                t?.Join();
            }

            Chromosome best = _levels.Select(l => l.BestChromosome).OrderBy(Fitness.Evaluate).First();

            return best;
        }

        public void AddToBucket(Chromosome chromosome, int prefLevel)
        {
            _buckets[prefLevel].TryAdd(chromosome);
        }

        private void PresolverWorker(CancellationToken token, bool tryCriticalSets)
        {
            bool controller(Solution s)
            {
                while (PresolversWaiting && !token.IsCancellationRequested)
                {
                    Thread.Sleep(50);
                }

                return true;
            }

            var baseSolver = new GreedySolver();
            var presolver = new PrefPumpPresolver();

            CriticalSetAnalysis cs = tryCriticalSets ? _criticalSetAnalysis : CriticalSetAnalysis.Empty(InputData);

            presolver.Presolve(
                InputData,
                baseSolver.SolveIndefinitely(InputData, cs, token)
                    .TakeWhile(controller)
                    .Where(s => Fitness.IsFeasible(Chromosome.FromOutput(InputData, s))),
                Fitness,
                token,
                (r, pref) =>
                {
                    if (Fitness.IsFeasible(r))
                    {
                        AddToBucket(r, pref);
                    }
                });
        }

        private void GaWorker(CancellationToken token, int level)
        {
            var ga = new GaLevel(this, level, _buckets[level]);

            _levels[LevelIndex(level)] = ga;

            ga.Run(token);
        }

        private void PrefPumpWorker(CancellationToken ct)
        {
            while (_levels.Any(l => l == null))
            {
                Thread.Sleep(500);
            }

            Status.Info(
                $"Begin preference pumping (Timeout {Options.PreferencePumpTimeoutSeconds}s, Depth {(Options.PreferencePumpMaxDepth < 0 ? "any" : Options.PreferencePumpMaxDepth.ToString())}).");

            while (!ct.IsCancellationRequested)
            {
                _prefPumpMinFound = Math.Min(_prefPumpMinFound, FirstRunningLevel);
                foreach (GaLevel level in _levels.Where(l => l.HasStarted))
                {
                    Chromosome[] chromosomes = level?.CurrentGeneration?.Select(c => c.Clone())
                        ?.Where(Fitness.IsFeasible)?.ToArray();
                    if (chromosomes == null)
                    {
                        Thread.Sleep(100);
                        continue;
                    }

                    foreach (Chromosome chromosome in chromosomes)
                    {
                        if (ct.IsCancellationRequested)
                        {
                            return;
                        }

                        PrefPumpResult result = PrefPumpHeuristic.TryPump(
                            chromosome,
                            level.PreferenceLevel,
                            Options.PreferencePumpMaxDepth,
                            TimeSpan.FromSeconds(Options.PreferencePumpTimeoutSeconds));
                        if (result == PrefPumpResult.Partial || result == PrefPumpResult.Success)
                        {
                            if (result == PrefPumpResult.Success && chromosome.MaxUsedPreference < _prefPumpMinFound)
                            {
                                Status.Info(
                                    $"Preference Pump Heuristic found better preference limit ({chromosome.MaxUsedPreference}).");
                                _prefPumpMinFound = chromosome.MaxUsedPreference;
                            }

                            while (!_buckets[chromosome.MaxUsedPreference].TryAdd(chromosome))
                            {
                                _buckets[chromosome.MaxUsedPreference].Take();
                            }
                        }
                    }
                }
            }
        }
    }
}