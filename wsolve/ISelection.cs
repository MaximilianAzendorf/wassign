namespace WSolve
{
    using System.Collections.Generic;

    public interface ISelection
    {
        IEnumerable<Chromosome> Select(int number, GaLevel level, IEnumerable<Chromosome> list);
    }
}
