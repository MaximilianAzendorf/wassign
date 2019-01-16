using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

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
        
        public IParameter<bool> Terminate { get; set; }
        
        public MutationCollection Mutations { get; } = new MutationCollection();
        
        public IReadOnlyList<ChromosomeList> Generations { get; }

        private readonly List<ChromosomeList> _generations = new List<ChromosomeList>();

        public ChromosomeList CurrentGeneration => Generations.LastOrDefault();

        public Chromosome BestChromosome => CurrentGeneration?.OrderBy(CurrentGeneration.GetFitness).First() ?? throw new InvalidOperationException();
        public (float, float) BestFitness => CurrentGeneration?.GetFitness(BestChromosome) ?? (float.MaxValue, float.MaxValue);
    
        public GeneticAlgorithm()
        {
            Generations = _generations.AsReadOnly();
        }
        
        public void Run(IEnumerable<Chromosome> initialPopulation)
        {
            _generations.Add(new ChromosomeList(Fitness, initialPopulation.OrderBy(Fitness.Evaluate)));

            while (!Terminate.Evalutate(this))
            {
                var newGen = AdvanceGeneration(_generations.Last());
                _generations.Add(newGen);
            }
        }

        private ChromosomeList AdvanceGeneration(ChromosomeList list)
        {
            list = new ChromosomeList(Fitness, list.Select(c => new Chromosome(c)));
            ChromosomeList newGen = new ChromosomeList(Fitness);

            int populationSize = PopulationSize.Evalutate(this);
            int selectionSize = SelectionSize.Evalutate(this);
            float crossChance = CrossoverChance.Evalutate(this);
            float mutChance = MutationChance.Evalutate(this);

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
                 
            newGen.AddRange(Reinsertion.Reinsert(populationSize, selected, offspring));
            
            foreach (Chromosome chromosome in newGen)
            {
                if (RNG.NextFloat() < mutChance)
                {
                    Mutate(chromosome);
                }
            }
            
            var newNewGen = new ChromosomeList(Fitness, newGen.OrderBy(newGen.GetFitness));
            newNewGen.InheritFitnessMap(newGen);
            newGen = newNewGen;
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