using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace WSolve
{
    [DebuggerDisplay("GaLevel [{" + nameof(PreferenceLevel) + "}]")]
    public class GaLevel
    {
        public int StartLimit => 1;//Math.Max(1, System.PopulationSize.Evalutate(System) / 25);
        public int GenerationsPassed { get; private set; } = 0;
        public ChromosomeList CurrentGeneration { get; private set; }
        
        public (int Full, int Partial, int Fail) PrefPumpSuccessStats { get; private set; }

        public Chromosome BestChromosome => CurrentGeneration?.OrderBy(System.Fitness.Evaluate)?.FirstOrDefault() ?? Chromosome.Null;
        public (float, float) BestFitness => System.Fitness.Evaluate(BestChromosome);

        private int _gensSinceLastImprovement = 0;

        public MultiLevelGaSystem System { get; }
        public int PreferenceLevel { get; }

        public ParallelOptions ParallelOptions => new ParallelOptions()
            {MaxDegreeOfParallelism = PreferenceLevel == System.FirstRunningLevel ? System.NumberofSubthreads : 1};

        public bool HasStarted { get; private set; } = false;

        public bool HasFinished { get; private set; } = false;

        public BlockingCollection<Chromosome> Bucket { get; }

        private int _insuffBucketGens = 0;
        
        public GaLevel(MultiLevelGaSystem system, int level, BlockingCollection<Chromosome> bucket)
        {
            System = system;
            PreferenceLevel = level;
            Bucket = bucket;
        }
        
        public void Run(CancellationToken ct)
        {
            HasFinished = false;
            while (Bucket.Count < StartLimit && !ct.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }
            
            CurrentGeneration = new ChromosomeList(this);
            while (CurrentGeneration.Count < StartLimit && Bucket.TryTake(out var x) && !ct.IsCancellationRequested)
            {
                CurrentGeneration.Add(x);
            }

            HasStarted = true;

            while (DateTime.Now < System.TimeStarted + System.Timeout && !ct.IsCancellationRequested)
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

                if (PreferenceLevel != System.FirstRunningLevel)
                {
                    Thread.Sleep((System.LevelIndex(PreferenceLevel) - System.LevelIndex(System.FirstRunningLevel)) * 100);
                }
            }

            HasFinished = true;
        }

        public float CalculateDiversity()
        {
            float d = 0;
            var gen = CurrentGeneration;
            foreach(var c1 in gen)
            foreach (var c2 in gen)
                d += c1.Distance(c2);
            return d / (gen.Count * gen.Count);
        }
        
        private ChromosomeList AdvanceGeneration(ChromosomeList list)
        {
            ChromosomeList origList = list;
            PrefPumpSuccessStats = (0, 0, 0);
            
            list = new ChromosomeList(this, list.Select(c => new Chromosome(c)));
            
            list.Remove(BestChromosome);
            var best = BestChromosome.Copy();

            int populationSize = System.PopulationSize.Evalutate(System) / Math.Max(1, 1 + System.LevelIndex(PreferenceLevel) - System.LevelIndex(System.FirstRunningLevel));
            float crossChance = System.CrossoverChance.Evalutate(System);
            float mutChance = System.MutationChance.Evalutate(System);

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

            List<Chromosome> mutated = new List<Chromosome>();
            foreach(Chromosome chromosome in nextGenPool)
            {
                if (RNG.NextFloat() < mutChance)
                {
                    var copy = chromosome.Copy();
                    Mutate(copy);
                    mutated.Add(copy);
                }
            }
            nextGenPool.AddRange(mutated);

            ChromosomeList samePref = new ChromosomeList(this);

            foreach (var chromosome in nextGenPool)
            {
                if (!System.Fitness.IsFeasible(chromosome))
                    continue;
                
                int maxUsedPreference = chromosome.MaxUsedPreference;
                if (maxUsedPreference == PreferenceLevel)
                {
                    samePref.Add(chromosome);
                }
                else
                {
                    System.AddToBucket(chromosome, maxUsedPreference);
                }
            }
            
            for(int i = 0; i < Bucket.Count / 4 && Bucket.TryTake(out var x); i++)
            {
                samePref.Add(x);
            }
            
            var nextGen = new ChromosomeList(this, System.Selection.Select(populationSize, this, samePref.Where(f => System.Fitness.IsFeasible(f))));
            nextGen.Add(BestChromosome);
            
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