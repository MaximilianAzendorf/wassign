using System;
using System.Collections.Generic;

namespace wsolve
{
    public interface IGeneticAlgorithm
    {
        IFitness Fitness { get; set; }
        ICrossover Crossover { get; set; }
        MutationCollection Mutations { get; }
        IReadOnlyList<ChromosomeList> Generations { get; }
    }
}