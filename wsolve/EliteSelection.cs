using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public class EliteSelection : ISelection
    {
        public IEnumerable<Chromosome> Select(int number, ChromosomeList list)
        {
            return list.OrderBy(list.GetFitness).Take(number);
        }
    }
}