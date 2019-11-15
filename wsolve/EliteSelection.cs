using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class EliteSelection : ISelection
    {
        public IEnumerable<Chromosome> Select(int number, GaLevel level, IEnumerable<Chromosome> list)
        {
            return list.OrderBy(level.ParentSystem.Fitness.Evaluate).Take(number);
        }

        public override string ToString()
        {
            return "elite";
        }
    }
}