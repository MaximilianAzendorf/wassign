using System;
using System.Collections.Generic;
using System.Text;

namespace WSolve
{
    public interface ISelection
    {
        IEnumerable<Chromosome> Select(int number, GaLevel level, IEnumerable<Chromosome> list);
    }
}
