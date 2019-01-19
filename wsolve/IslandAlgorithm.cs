using System.Collections.Generic;

namespace wsolve
{
    public class IslandAlgorithm : IGeneticAlgorithm
    {
        public IFitness Fitness { get; set; }
        public ICrossover Crossover { get; set; }
        public ISelection Selection { get; set; }
        public IReinsertion Reinsertion { get; set; }
        public MutationCollection Mutations { get; }
        
        public IParameter<int> PopulationSize { get; set; }
        public IParameter<int> SelectionSize { get; set; }
        public IParameter<float> CrossoverChance { get; set; }
        public IParameter<float> MutationChance { get; set; }
        
        public IParameter<bool> Termination { get; set; }
        public IParameter<bool> DoCrossover { get; set; }
        
        public IReadOnlyList<ChromosomeList> Generations { get; }
        public float StagnationFactor { get; }

        public int IslandCount => _islands.Length;

        private readonly GeneticAlgorithm[] _islands;
        private IGeneticAlgorithm _geneticAlgorithmImplementation;

        public IslandAlgorithm(int numIslands)
        {
            _islands = new GeneticAlgorithm[numIslands];
        }
    }
}