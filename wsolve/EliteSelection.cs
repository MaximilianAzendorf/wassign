using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public class EliteSelection : ISelection
    {
        public IEnumerable<Chromosome> Select(int number, GaLevel level, IEnumerable<Chromosome> list)
        {
            return list.OrderBy(level.System.Fitness.Evaluate).Take(number);
        }
    }
}