using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    public class Generation : List<Chromosome>
    {
        public int Number { get; }

        public Generation(int number)
        {
            Number = number;
        }

        public Generation(int number, IEnumerable<Chromosome> initialPopulation)
            : base(initialPopulation)
        {
            Number = number;
        }
    }
}
