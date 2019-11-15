using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class ChromosomeList : List<Chromosome>
    {
        private readonly GaLevel _level;

        public ChromosomeList(GaLevel level)
        {
            _level = level;
        }

        public ChromosomeList(GaLevel level, IEnumerable<Chromosome> population)
            : base(population)
        {
            _level = level;
        }

#if DEBUG
        public IEnumerable<((float major, float minor) fitness, Chromosome chromosome)> DebugBestView =>
            this.Select(c => (_level.ParentSystem.Fitness.Evaluate(c), c)).OrderBy(x => x.Item1);
#endif
    }
}