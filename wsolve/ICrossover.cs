using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    public interface ICrossover
    {
        int ParentCount { get; }
        int ChildrenCount { get; }

        IEnumerable<Chromosome> Crossover(IReadOnlyList<Chromosome> parents);
    }
}
