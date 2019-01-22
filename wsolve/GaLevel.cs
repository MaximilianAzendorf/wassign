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
    [DebuggerDisplay("GaLevel [{" + nameof(PreferenceLevel) + "}]")]
    public class GaLevel
    {
        public int StartLimit => 1;//Math.Max(1, System.PopulationSize.Evalutate(this) / 25);
        public int GenerationsPassed { get; private set; } = 0;
        public ChromosomeList CurrentGeneration { get; private set; }
        
        public (int Full, int Partial, int Fail) PrefPumpSuccessStats { get; private set; }

        public Chromosome BestChromosome => CurrentGeneration?.OrderBy(System.Fitness.Evaluate)?.FirstOrDefault() ?? Chromosome.Null;
        public (float, float) BestFitness => System.Fitness.Evaluate(BestChromosome);

        private int _gensSinceLastImprovement = 0;

        public float StagnationFactor => (float)Math.Pow(Math.Max(0, Math.Min(1, _gensSinceLastImprovement / (50f + GenerationsPassed))), 2f/3f);
        public MultiLevelGaSystem System { get; }
        public int PreferenceLevel { get; }

        public ParallelOptions ParallelOptions => new ParallelOptions()
            {MaxDegreeOfParallelism = PreferenceLevel == System.FirstRunningLevel ? System.NumberofSubthreads : 1};

        public bool HasStarted { get; private set; } = false;

        public bool HasFinished { get; private set; } = false;

        private BlockingCollection<Chromosome> _bucket;

        private int reservedBucketSpace(int populationSize) => Math.Min(_bucket.Count, populationSize / 4);

        private int _insuffBucketGens = 0;
        
        public GaLevel(MultiLevelGaSystem system, int level, BlockingCollection<Chromosome> bucket)
        {
            System = system;
            PreferenceLevel = level;
            _bucket = bucket;
        }
        
        public void Run(CancellationToken ct)
        {
            HasFinished = false;
            while (_bucket.Count < StartLimit && !ct.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }
            
            CurrentGeneration = new ChromosomeList(this);
            while (CurrentGeneration.Count < StartLimit && _bucket.TryTake(out var x) && !ct.IsCancellationRequested)
            {
                CurrentGeneration.Add(x);
            }

            HasStarted = true;

            while (!System.Termination.Evalutate(this) && !ct.IsCancellationRequested)
            {
                var newGen = AdvanceGeneration(CurrentGeneration);
                var oldBestFitness = BestFitness;
                CurrentGeneration = newGen;
                
                if (BestFitness == oldBestFitness)
                {
                    _gensSinceLastImprovement++;
                }
                else
                {
                    _gensSinceLastImprovement = 0;
                }
            }

            HasFinished = true;
        }
        
        private ChromosomeList AdvanceGeneration(ChromosomeList list)
        {
            ChromosomeList origList = list;
            PrefPumpSuccessStats = (0, 0, 0);
            
            list = new ChromosomeList(this, list.Select(c => new Chromosome(c)));

            var best = BestChromosome.Copy();

            int populationSize = System.PopulationSize.Evalutate(this) / Math.Max(1, 1 + PreferenceLevel - System.FirstRunningLevel);
            float crossChance = System.CrossoverChance.Evalutate(this);
            float mutChance = System.MutationChance.Evalutate(this);
            float prpumpChance = System.PrefPumpChance.Evalutate(this);
            int prpumpDepth = System.PrefPumpDepth.Evalutate(this);
            TimeSpan prpumpTimeout = System.PrefPumpTimeout.Evalutate(this);

            ChromosomeList nextGenPool = new ChromosomeList(this, list);
            
            Parallel.For(0, nextGenPool.Count / System.Crossover.ParentCount, ParallelOptions, rawi =>
            {
                int i = rawi * System.Crossover.ParentCount;
                if (RNG.NextFloat() <= crossChance)
                {
                    List<Chromosome> parents = new List<Chromosome>(System.Crossover.ParentCount);
                    for (int j = i; j < i + System.Crossover.ParentCount; j++)
                    {
                        parents.Add(nextGenPool[j]);
                    }

                    var res = System.Crossover.Crossover(parents);
                    lock(nextGenPool) nextGenPool.AddRange(res);
                }
            });

            foreach(Chromosome chromosome in nextGenPool)
            {
                if (RNG.NextFloat() < mutChance)
                {
                    Mutate(chromosome);
                }

                if (RNG.NextFloat() < prpumpChance)
                {
                    Input input = chromosome.Input;
                    int pumpPref = -1;
                    for (int i = 0; i <= input.MaxPreference; i++)
                    {
                        int prefCount = chromosome.CountPreference(i);
                        if (prefCount == 0) continue;
                        pumpPref = i;
                    }

                    if (pumpPref != -1)
                    {
                        var r = PrefPumpHeuristic.TryPump(chromosome, pumpPref, prpumpDepth, prpumpTimeout);
                        if (r == PrefPumpResult.Partial)
                            PrefPumpSuccessStats = (PrefPumpSuccessStats.Full, PrefPumpSuccessStats.Partial + 1,
                                PrefPumpSuccessStats.Fail);
                        if (r == PrefPumpResult.Success)
                            PrefPumpSuccessStats = (PrefPumpSuccessStats.Full + 1, PrefPumpSuccessStats.Partial,
                                PrefPumpSuccessStats.Fail);
                        if (r == PrefPumpResult.Fail)
                            PrefPumpSuccessStats = (PrefPumpSuccessStats.Full, PrefPumpSuccessStats.Partial,
                                PrefPumpSuccessStats.Fail + 1);
                    }
                }
            }

            ChromosomeList samePref = new ChromosomeList(this);

            foreach (var chromosome in nextGenPool)
            {
                int maxUsedPreference = chromosome.MaxUsedPreference;
                if (maxUsedPreference == PreferenceLevel)
                {
                    samePref.Add(chromosome);
                }
                else
                {
                    if(System.Fitness.IsFeasible(chromosome))
                        System.AddToBucket(chromosome, maxUsedPreference);
                }
            }
            
            var nextGen = new ChromosomeList(this, System.Selection.Select(populationSize - reservedBucketSpace(populationSize), this, samePref.Where(f => System.Fitness.IsFeasible(f))));
            nextGen.Add(BestChromosome);
            
            while (nextGen.Count < populationSize && _bucket.TryTake(out Chromosome x))
            {
                nextGen.Add(x);
            }

            if (nextGen.Count < populationSize && nextGen.Count > 0)
            {
                _insuffBucketGens++;
                Thread.Sleep(100 * _insuffBucketGens);
            }
            else
            {
                _insuffBucketGens = 0;
            }

            GenerationsPassed++;
            return nextGen;
        }

        private void Mutate(Chromosome chromosome)
        {
            var table = System.Mutations.GetSelectionSnapshot();

            float s = RNG.NextFloat();

            IMutation m = table[table.Length - 1].mutation;
            for (int i = 0; i < table.Length; i++)
            {
                s -= table[i].cost;
                if (!(s <= 0f)) continue;
                
                m = table[i].mutation;
                break;
            }
            
            m.Mutate(chromosome);
        }
    }
}