using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    public interface ISelection
    {
        IEnumerable<Chromosome> Select(int number, ChromosomeList list);
    }
}
