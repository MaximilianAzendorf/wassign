using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    interface ISelection
    {
        IEnumerable<Chromosome> Select(int number, IReadOnlyList<Chromosome> chromosomes);
    }
}
