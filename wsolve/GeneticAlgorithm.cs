using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    public class GeneticAlgorithm<TFitness>
        where TFitness : IComparable<TFitness>
    {
        private IFitness<TFitness> _fitness;
        private ICrossover _crossover;

        public IFitness<TFitness> Fitness
        {
            get => _fitness;
            set => _fitness = value ?? throw new ArgumentNullException(nameof(value));
        }

        public ICrossover Crossover
        {
            get => _crossover;
            set => _crossover = value ?? throw new ArgumentNullException(nameof(value));
        }

        public MutationCollection Mutations { get; } = new MutationCollection();

        private List<Generation> generations { get; } = new List<Generation>();
    
        public GeneticAlgorithm(int populationSize, IEnumerable<Chromosome> initialPopulation)
        {
            generations.Add(new Generation(generations.Count, initialPopulation));
        }
    }
}
