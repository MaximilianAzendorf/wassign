using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace wsolve
{
    public class ChromosomeList : List<Chromosome>
    {
        private GaLevel _level;

#if DEBUG
        public IEnumerable<((float, float), Chromosome)> DebugBestView =>
            this.Select(c => (_level.System.Fitness.Evaluate(c), c)).OrderBy(x => x.Item1);
#endif
        
        public ChromosomeList(GaLevel level)
        {
            _level = level;
        }

        public ChromosomeList(GaLevel level, IEnumerable<Chromosome> population)
            : base(population)
        {
            _level = level;
        }
    }
}
