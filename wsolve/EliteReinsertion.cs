using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public class EliteReinsertion : IReinsertion
    {
        public IEnumerable<Chromosome> Reinsert(int number, ChromosomeList parents, ChromosomeList offspring)
        {
            int diff = number - offspring.Count;
            
            return diff > 0
                ? offspring.Concat(parents.OrderBy(parents.GetFitness).Take(diff))
                : offspring.OrderBy(offspring.GetFitness).Take(number);
        }
    }
}