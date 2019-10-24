using System.Collections.Generic;

namespace WSolve
{
    public interface ISelection
    {
        IEnumerable<Chromosome> Select(int number, GaLevel level, IEnumerable<Chromosome> list);
    }
}