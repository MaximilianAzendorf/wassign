namespace WSolve
{
    using System.Collections.Generic;

    public interface ICrossover
    {
        int ParentCount { get; }
        
        int ChildrenCount { get; }

        IEnumerable<Chromosome> Crossover(IReadOnlyList<Chromosome> parents);
    }
}
