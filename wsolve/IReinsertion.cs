using System.Collections;
using System.Collections.Generic;

namespace wsolve
{
    public interface IReinsertion
    {
        IEnumerable<Chromosome> Reinsert(int number, ChromosomeList parents, ChromosomeList offspring);
    }
}