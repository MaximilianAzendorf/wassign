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
        private int _insuffBucketGens;

        public GaLevel(MultiLevelGaSystem system, int level, BlockingCollection<Chromosome> bucket)
        {
            System = system;
            PreferenceLevel = level;
            Bucket = bucket;
        }

        public int StartLimit => 1;

        public int GenerationsPassed { get; private set; }

        public ChromosomeList CurrentGeneration { get; private set; }

        public Chromosome BestChromosome =>
            CurrentGeneration?.OrderBy(System.Fitness.Evaluate)?.FirstOrDefault() ?? Chromosome.Null;

        public (float major, float minor) BestFitness => System.Fitness.Evaluate(BestChromosome);

        public MultiLevelGaSystem System { get; }

        public int PreferenceLevel { get; }

        public ParallelOptions ParallelOptions => new ParallelOptions
        {
            MaxDegreeOfParallelism = PreferenceLevel == System.FirstRunningLevel ? System.NumberofSubthreads : 1
        };

        public bool HasStarted { get; private set; }

        public bool HasFinished { get; private set; }

        public BlockingCollection<Chromosome> Bucket { get; }

        public void Run(CancellationToken ct)
        {
            HasFinished = false;
            while (Bucket.Count < StartLimit && !ct.IsCancellationRequested) Thread.Sleep(50);

            CurrentGeneration = new ChromosomeList(this);
            while (CurrentGeneration.Count < StartLimit && Bucket.TryTake(out var x) && !ct.IsCancellationRequested)
                CurrentGeneration.Add(x);

            HasStarted = true;

            while (DateTime.Now < System.TimeStarted + System.Timeout && !ct.IsCancellationRequested)
            {
                var newGen = AdvanceGeneration(CurrentGeneration);
                var oldBestFitness = BestFitness;
                CurrentGeneration = newGen;

                if (BestFitness == oldBestFitness)
                {
                }

                if (PreferenceLevel != System.FirstRunningLevel)
                    Thread.Sleep((System.LevelIndex(PreferenceLevel) - System.LevelIndex(System.FirstRunningLevel)) *
                                 100);
            }

            HasFinished = true;
        }

        private ChromosomeList AdvanceGeneration(ChromosomeList list)
        {
            list = new ChromosomeList(this, list.Select(c => new Chromosome(c)));

            list.Remove(BestChromosome);

            var populationSize = System.PopulationSize.Evalutate(System) / Math.Max(1,
                                     1 + System.LevelIndex(PreferenceLevel) -
                                     System.LevelIndex(System.FirstRunningLevel));
            var crossChance = System.CrossoverChance.Evalutate(System);
            var mutChance = System.MutationChance.Evalutate(System);

            var nextGenPool = new ChromosomeList(this, list);

            Parallel.For(0, nextGenPool.Count / System.Crossover.ParentCount, ParallelOptions, rawi =>
            {
                var i = rawi * System.Crossover.ParentCount;
                if (RNG.NextFloat() <= crossChance)
                {
                    var parents = new List<Chromosome>(System.Crossover.ParentCount);
                    for (var j = i; j < i + System.Crossover.ParentCount; j++) parents.Add(nextGenPool[j]);

                    var res = System.Crossover.Crossover(parents);
                    lock (nextGenPool)
                    {
                        nextGenPool.AddRange(res);
                    }
                }
            });

            var mutated = new List<Chromosome>();
            foreach (var chromosome in nextGenPool)
                if (RNG.NextFloat() < mutChance)
                {
                    var copy = chromosome.Copy();
                    Mutate(copy);
                    mutated.Add(copy);
                }

            nextGenPool.AddRange(mutated);

            var samePref = new ChromosomeList(this);

            foreach (var chromosome in nextGenPool)
            {
                if (!System.Fitness.IsFeasible(chromosome)) continue;

                var maxUsedPreference = chromosome.MaxUsedPreference;
                if (maxUsedPreference == PreferenceLevel)
                    samePref.Add(chromosome);
                else
                    System.AddToBucket(chromosome, maxUsedPreference);
            }

            for (var i = 0; i < Bucket.Count / 4 && Bucket.TryTake(out var x); i++) samePref.Add(x);

            var nextGen = new ChromosomeList(this,
                System.Selection.Select(populationSize, this, samePref.Where(f => System.Fitness.IsFeasible(f))));
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

            var s = RNG.NextFloat();

            var m = table[table.Length - 1].mutation;
            for (var i = 0; i < table.Length; i++)
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