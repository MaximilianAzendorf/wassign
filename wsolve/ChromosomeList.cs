using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace wsolve
{
    public class ChromosomeList : List<Chromosome>
    {
        private readonly IFitness _fitness;

        private Dictionary<Chromosome, (float, float)> _fitnessMap = new Dictionary<Chromosome, (float, float)>();

#if DEBUG
        public IEnumerable<((float, float), Chromosome)> DebugBestView =>
            this.Select(c => (GetFitness(c), c)).OrderBy(x => x.Item1);
#endif
        
        public ChromosomeList(IFitness fitness)
        {
            _fitness = fitness;
        }

        public ChromosomeList(IFitness fitness, IEnumerable<Chromosome> population)
            : base(population)
        {
            _fitness = fitness;
        }

        public (float, float) GetFitness(Chromosome chromosome)
        {
            lock (_fitnessMap)
            {
                if (!_fitnessMap.TryGetValue(chromosome, out var f))
                {
                    f = _fitness.Evaluate(chromosome);
                    _fitnessMap.Add(chromosome, f);
                }

                return f;
            }
        }

        internal void InheritFitnessMap(ChromosomeList otherList)
        {
            _fitnessMap = new Dictionary<Chromosome, (float, float)>(otherList._fitnessMap);
        }
    }
}
