using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace WSolve
{
    [DebuggerDisplay("GaLevel [{" + nameof(PreferenceLevel) + "}]")]
    public class GaLevel
    {
        private int _insufficientBucketGens;

        public GaLevel(MultiLevelGaSystem parentSystem, int level, BlockingCollection<Chromosome> bucket)
        {
            ParentSystem = parentSystem;
            PreferenceLevel = level;
            Bucket = bucket;
        }

        public int GenerationsPassed { get; private set; }

        public ChromosomeList CurrentGeneration { get; private set; }

        public Chromosome BestChromosome =>
            CurrentGeneration?.OrderBy(ParentSystem.Fitness.Evaluate)?.FirstOrDefault() ?? Chromosome.Null;

        public (float major, float minor) BestFitness => ParentSystem.Fitness.Evaluate(BestChromosome);

        public MultiLevelGaSystem ParentSystem { get; }

        public int PreferenceLevel { get; }

        public ParallelOptions ParallelOptions => new ParallelOptions
        {
            MaxDegreeOfParallelism =
                PreferenceLevel == ParentSystem.FirstRunningLevel ? ParentSystem.NumberofSubthreads : 1
        };

        public bool HasStarted { get; private set; }

        public bool HasFinished { get; private set; }

        public BlockingCollection<Chromosome> Bucket { get; }

        public void Run(CancellationToken ct)
        {
            HasFinished = false;
            while (Bucket.Count == 0 && !ct.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }

            CurrentGeneration = new ChromosomeList(this);
            while (CurrentGeneration.Count < 1 && Bucket.TryTake(out Chromosome x) && !ct.IsCancellationRequested)
            {
                CurrentGeneration.Add(x);
            }

            HasStarted = true;

            while (DateTime.Now < ParentSystem.TimeStarted + ParentSystem.Timeout && !ct.IsCancellationRequested)
            {
                ChromosomeList newGen = AdvanceGeneration(CurrentGeneration);
                (float major, float minor) oldBestFitness = BestFitness;
                CurrentGeneration = newGen;

                if (BestFitness == oldBestFitness) { }

                if (PreferenceLevel != ParentSystem.FirstRunningLevel)
                {
                    Thread.Sleep((ParentSystem.LevelIndex(PreferenceLevel) -
                                  ParentSystem.LevelIndex(ParentSystem.FirstRunningLevel)) *
                                 100);
                }
            }

            HasFinished = true;
        }

        private ChromosomeList AdvanceGeneration(ChromosomeList list)
        {
            list = new ChromosomeList(this, list.Select(c => new Chromosome(c)));

            list.Remove(BestChromosome);

            int populationSize = ParentSystem.PopulationSize.Evalutate(ParentSystem) / Math.Max(1,
                                     1 + ParentSystem.LevelIndex(PreferenceLevel) -
                                     ParentSystem.LevelIndex(ParentSystem.FirstRunningLevel));
            float crossChance = ParentSystem.CrossoverChance.Evalutate(ParentSystem);
            float mutChance = ParentSystem.MutationChance.Evalutate(ParentSystem);

            var nextGenPool = new ChromosomeList(this, list);

            Parallel.For(0, nextGenPool.Count / ParentSystem.Crossover.ParentCount, ParallelOptions, rawi =>
            {
                int i = rawi * ParentSystem.Crossover.ParentCount;
                if (RNG.NextFloat() <= crossChance)
                {
                    var parents = new List<Chromosome>(ParentSystem.Crossover.ParentCount);
                    for (int j = i; j < i + ParentSystem.Crossover.ParentCount; j++)
                    {
                        parents.Add(nextGenPool[j]);
                    }

                    IEnumerable<Chromosome> res = ParentSystem.Crossover.Crossover(parents);
                    lock (nextGenPool)
                    {
                        nextGenPool.AddRange(res);
                    }
                }
            });

            var mutated = new List<Chromosome>();
            foreach (Chromosome chromosome in nextGenPool)
            {
                if (RNG.NextFloat() < mutChance)
                {
                    Chromosome copy = chromosome.Clone();
                    Mutate(copy);
                    mutated.Add(copy);
                }
            }

            nextGenPool.AddRange(mutated);

            var samePref = new ChromosomeList(this);

            foreach (Chromosome chromosome in nextGenPool)
            {
                if (!ParentSystem.Fitness.IsFeasible(chromosome))
                {
                    continue;
                }

                int maxUsedPreference = chromosome.MaxUsedPreference;
                if (maxUsedPreference == PreferenceLevel)
                {
                    samePref.Add(chromosome);
                }
                else
                {
                    ParentSystem.AddToBucket(chromosome, maxUsedPreference);
                }
            }

            for (int i = 0; i < Bucket.Count / 4 && Bucket.TryTake(out Chromosome x); i++)
            {
                samePref.Add(x);
            }

            var nextGen = new ChromosomeList(this,
                ParentSystem.Selection.Select(populationSize, this,
                    samePref.Where(f => ParentSystem.Fitness.IsFeasible(f))));
            nextGen.Add(BestChromosome);

            if (nextGen.Count < populationSize && nextGen.Count > 0)
            {
                _insufficientBucketGens++;
                Thread.Sleep(100 * _insufficientBucketGens);
            }
            else
            {
                _insufficientBucketGens = 0;
            }

            GenerationsPassed++;
            return nextGen;
        }

        private void Mutate(Chromosome chromosome)
        {
            (float cost, IMutation mutation)[] table = ParentSystem.Mutations.GetSelectionSnapshot();

            float s = RNG.NextFloat();

            IMutation m = table[table.Length - 1].mutation;
            for (int i = 0; i < table.Length; i++)
            {
                s -= table[i].cost;
                if (!(s <= 0f))
                {
                    continue;
                }

                m = table[i].mutation;
                break;
            }

            m.Mutate(chromosome);
        }
    }
}