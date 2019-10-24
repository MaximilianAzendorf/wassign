using System.Collections.Generic;

namespace WSolve
{
    public interface ICrossover
    {
        int ParentCount { get; }

        int ChildrenCount { get; }

        IEnumerable<Chromosome> Crossover(IReadOnlyList<Chromosome> parents);
    }
}