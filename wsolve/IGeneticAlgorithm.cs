using System;
using System.Collections.Generic;

namespace wsolve
{
    public interface IGeneticAlgorithm
    {
        IFitness Fitness { get; set; }
        ICrossover Crossover { get; set; }
        ISelection Selection { get; set; }
        IReinsertion Reinsertion { get; set; }
        MutationCollection Mutations { get; }
        IReadOnlyList<ChromosomeList> Generations { get; }
        
        float StagnationFactor { get; }
    }
}