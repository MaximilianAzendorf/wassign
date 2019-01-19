using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace wsolve
{
    public class GeneticAlgorithm : IGeneticAlgorithm
    {
        public IParameter<int> GenerationSize { get; set; }

        public ISelection Selection { get; set; }
        
        public IFitness Fitness { get; set; }

        public ICrossover Crossover { get; set; }
        
        public IReinsertion Reinsertion { get; set; }

        public IParameter<int> PopulationSize { get; set; }
        public IParameter<int> SelectionSize { get; set; }
        public IParameter<float> CrossoverChance { get; set; }
        public IParameter<float> MutationChance { get; set; }
        public IParameter<bool> Termination { get; set; }
        public IParameter<float> PrefPumpChance { get; set; }
        
        public IParameter<TimeSpan> PrefPumpTimeout { get; set; }
        public IParameter<int> PrefPumpDepth { get; set; }
        
        public MutationCollection Mutations { get; } = new MutationCollection();
        
        public IReadOnlyList<ChromosomeList> Generations { get; }

        private readonly List<ChromosomeList> _generations = new List<ChromosomeList>();

        public ChromosomeList CurrentGeneration => Generations.LastOrDefault();
        
        public (int Full, int Partial, int Fail) PrefPumpSuccessStats { get; private set; }

        public Chromosome BestChromosome => CurrentGeneration?.OrderBy(CurrentGeneration.GetFitness).First() ?? throw new InvalidOperationException();
        public (float, float) BestFitness => CurrentGeneration?.GetFitness(BestChromosome) ?? (float.MaxValue, float.MaxValue);

        private int _gensSinceLastImprovement = 0;

        public float StagnationFactor => (float)Math.Pow(Math.Max(0, Math.Min(1, _gensSinceLastImprovement / (50f + Generations.Count))), 2f/3f);
        
        public GeneticAlgorithm()
        {
            Generations = _generations.AsReadOnly();
        }
        
        private IEnumerable<Chromosome> Preprocess(IEnumerable<Chromosome> population)
        {
            ConcurrentBag<Chromosome> finished = new ConcurrentBag<Chromosome>();
            ConcurrentBag<Chromosome> pool = new ConcurrentBag<Chromosome>(population);
            var input = pool.First().Input;
            
            Status.Info("Preprocessing: Long-term preference pumping.");
            for (int i = input.MaxPreference; i > 0 && pool.Any(); i--)
            {
                ConcurrentBag<Chromosome> newPool = new ConcurrentBag<Chromosome>();
                Status.Info($"Preference pumping for pref={i} (Pool: {pool.Count}).");
                Parallel.ForEach(pool, c =>
                {
                    if (PrefPumpHeuristic.TryPump(c, i, 5, TimeSpan.FromMilliseconds(100)) == PrefPumpResult.SUCCESS)
                    {
                        newPool.Add(c);
                    }
                    else
                    {
                        finished.Add(c);
                    }
                });

                pool = newPool;
            }
            
            Status.Info("Finishing preprocessing...");
            return finished.OrderBy(Fitness.Evaluate).ToArray();
        }
        
        public void Run(IEnumerable<Chromosome> initialPopulation)
        {
            //initialPopulation = Preprocess(initialPopulation);
            
            _generations.Add(new ChromosomeList(Fitness, initialPopulation.Take(PopulationSize.Evalutate(this)).OrderBy(Fitness.Evaluate)));

            while (!Termination.Evalutate(this))
            {
                var newGen = AdvanceGeneration(_generations.Last());
                var oldBestFitness = BestFitness;
                _generations.Add(newGen);
                if (BestFitness == oldBestFitness)
                {
                    _gensSinceLastImprovement++;
                }
                else
                {
                    _gensSinceLastImprovement = 0;
                }

                if (DateTime.Now - _lastStatusOut > TimeSpan.FromSeconds(1))
                {
                    Status.Info(
                        $"GEN #{Generations.Count.ToString().PadRight(6)} ({(CurrentGeneration?.Count + "):").PadRight(8)} Best=({BestFitness.Item1}|{BestFitness.Item2:0.0000000}), Stag={StagnationFactor:0.000}, PrPump={PrefPumpSuccessStats}");
                    _lastStatusOut = DateTime.Now;
                    Task.Delay(250).Wait();
                }
            }
        }

        private DateTime _lastStatusOut = DateTime.MinValue;
        
        private ChromosomeList AdvanceGeneration(ChromosomeList list)
        {
            ChromosomeList origList = list;
            PrefPumpSuccessStats = (0, 0, 0);
            
            list = new ChromosomeList(Fitness, list.Select(c => new Chromosome(c)));
            list.InheritFitnessMap(origList);

            foreach (Chromosome chromosome in list.Where(c => list.GetFitness(c) != (float.PositiveInfinity, float.PositiveInfinity)))
            {
            }
            
            ChromosomeList newGen = new ChromosomeList(Fitness);
            var best = BestChromosome.Copy();

            int populationSize = PopulationSize.Evalutate(this);
            int selectionSize = SelectionSize.Evalutate(this);
            float crossChance = CrossoverChance.Evalutate(this);
            float mutChance = MutationChance.Evalutate(this);
            float prpumpChance = PrefPumpChance.Evalutate(this);
            int prpumpDepth = PrefPumpDepth.Evalutate(this);
            TimeSpan prpumpTimeout = PrefPumpTimeout.Evalutate(this);

            ChromosomeList selected = new ChromosomeList(Fitness, 
                Selection.Select(selectionSize, list).OrderBy(x => RNG.NextInt()));
            ChromosomeList offspring = new ChromosomeList(Fitness);

            selected.InheritFitnessMap(list);
            offspring.InheritFitnessMap(list);

            for (int i = 0; i < selected.Count - Crossover.ParentCount + 1; i += Crossover.ParentCount)
            {
                if (RNG.NextFloat() <= crossChance)
                {
                    List<Chromosome> parents = new List<Chromosome>(Crossover.ParentCount);
                    for (int j = i; j < i + Crossover.ParentCount; j++)
                    {
                        parents.Add(selected[j]);
                    }
                    offspring.AddRange(Crossover.Crossover(parents));
                }
            }
                 
            newGen.AddRange(Reinsertion.Reinsert(populationSize, 
                selected, 
                offspring));
            
            foreach (Chromosome chromosome in newGen)
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
                        if (r == PrefPumpResult.PARTIAL)
                            PrefPumpSuccessStats = (PrefPumpSuccessStats.Full, PrefPumpSuccessStats.Partial + 1,
                                PrefPumpSuccessStats.Fail);
                        if (r == PrefPumpResult.SUCCESS)
                            PrefPumpSuccessStats = (PrefPumpSuccessStats.Full + 1, PrefPumpSuccessStats.Partial,
                                PrefPumpSuccessStats.Fail);
                        if (r == PrefPumpResult.FAIL)
                            PrefPumpSuccessStats = (PrefPumpSuccessStats.Full, PrefPumpSuccessStats.Partial,
                                PrefPumpSuccessStats.Fail + 1);
                    }
                }
            }
            
            var newNewGen = new ChromosomeList(Fitness, newGen.OrderBy(newGen.GetFitness));
            newNewGen.InheritFitnessMap(newGen);
            newGen = newNewGen;
            newGen.Add(best);
            
            return newGen;
        }

        private void Mutate(Chromosome chromosome)
            
        {
            var table = Mutations.GetSelectionSnapshot();

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