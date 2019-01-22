using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace wsolve
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
        public IParameter<bool> Termination { get; set; }
        public IParameter<float> PrefPumpChance { get; set; }
        
        public IParameter<TimeSpan> PrefPumpTimeout { get; set; }
        public IParameter<int> PrefPumpDepth { get; set; }
        
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

        public int NumberOfGeneticThreads => Input.MaxPreference;

        public int NumberofSubthreads { get; } = Environment.ProcessorCount / 2;
        public int NumberOfPresolverThreads { get; }
        public Input Input { get; }
        public int BucketLimit { get; }

        public int FirstRunningLevel => _levels.OrderBy(l => l.PreferenceLevel).FirstOrDefault(l => l.HasStarted)?.PreferenceLevel ?? 0;
        
        private CancellationTokenSource _cts = new CancellationTokenSource();

        public float StagnationFactor => _levels[FirstRunningLevel].StagnationFactor;

        private BlockingCollection<Chromosome>[] _buckets;
        private GaLevel[] _levels;
        private Thread[] _threads;
        
        public MultiLevelGaSystem(Input input, int bucketLimit)
        {
            BucketLimit = bucketLimit;   
            
            Input = input;
            NumberOfPresolverThreads = 1;
             
            _buckets = new BlockingCollection<Chromosome>[Input.MaxPreference+1];
            for (int i = 0; i < _buckets.Length; i++)
            {
                _buckets[i] = new BlockingCollection<Chromosome>(BucketLimit);
            }
        }

        public void Start()
        {
            Status.Info($"Starting GA System with {NumberOfPresolverThreads} presolver worker(s) and {NumberOfGeneticThreads} genetic workers.");

            _threads = new Thread[NumberOfPresolverThreads + Input.MaxPreference+1];

            _levels = new GaLevel[Input.MaxPreference+1];
            
            for (int i = 0; i < NumberOfPresolverThreads; i++)
            {
                //_threads[i + Input.MaxPreference] = Task.Run(() => PresolverWorker(_cts.Token));
                _threads[i + Input.MaxPreference] = new Thread(() => PresolverWorker(_cts.Token));
                _threads[i + Input.MaxPreference].Start();
            }
            
            for (int i = 0; i <= Input.MaxPreference; i++)
            {
                int closurei = i;
                //_threads[i] = Task.Run(() => GaWorker(_cts.Token, closurei, NumberOfGeneticThreads / Environment.ProcessorCount));
                _threads[i] = new Thread(() => GaWorker(_cts.Token, closurei, NumberOfGeneticThreads / Environment.ProcessorCount));
                _threads[i].Start();
            }
        }

        public Output WaitForSolution(TimeSpan outputFrequency)
        {
            while (_levels.All(l => !l.HasFinished))
            {
                Thread.Sleep(outputFrequency.Milliseconds);

                if (_levels.All(l => !l.HasStarted)) continue;
                
                StringBuilder state = new StringBuilder();
                
                state.Append($"BEST=({(int)BestFitness.major}, {BestFitness.minor:0.00000}); STAG={StagnationFactor:0.000}; BUCKETS:  ");
                
                int minStarted = _levels.Where(l => l.HasStarted).Min(l => l.PreferenceLevel);
                minStarted = Math.Min(_levels.Length - 2, Math.Max(1, minStarted));
                
                for(int i = minStarted - 1; i <= minStarted + 1; i++)
                {
                    GaLevel level = _levels[i];
                    if (level == null) continue;
                    string status = $" [{i}({_levels[i].GenerationsPassed}):{level.CurrentGeneration?.Count ?? 0}/{_buckets[i].Count}] ";
                    state.Append(status);
                }

                Status.Info(state.ToString());
            }
            
            Status.Info("Stopped GA System. Waiting for workers to finish.");
            _cts.Cancel();
            foreach (var t in _threads) t?.Join();

            var best = _levels.Select(l => l.BestChromosome).OrderBy(Fitness.Evaluate).First();
            
            return new Output(
                Enumerable.Range(0, Input.Workshops.Count).Select(w => (w, best.Slot(w))),
                Enumerable.Range(0, Input.Participants.Count).SelectMany(p =>
                    Enumerable.Range(0, Input.Slots.Count).Select(s => (p, best.Workshop(p,s)))));
        }

        public void AddToBucket(Chromosome chromosome, int prefLevel)
        {
            if (!Fitness.IsFeasible(chromosome)) ((GaSolverFitness)Fitness).IsFeasible(chromosome, true);
            _buckets[prefLevel].TryAdd(chromosome);
        }

        private void PresolverWorker(CancellationToken token)
        {
            GreedySolver baseSolver = new GreedySolver();
            PrefPumpPresolver presolver = new PrefPumpPresolver();
            
            presolver.Presolve(Input, baseSolver.SolveIndefinitely(Input, token), token, AddToBucket);
        }

        private void GaWorker(CancellationToken token, int level, int threads)
        {
            GaLevel ga = new GaLevel(this, level, _buckets[level]);

            _levels[level] = ga;
            
            ga.Run(token);
        }
    }
}