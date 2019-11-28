using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions 
{
    public class Slot : SlotBase
    {
        private readonly Chromosome _chromosome;
        
        public Slot(int id, Chromosome chromosome)
            : base(id, chromosome.InputData)
        {
            _chromosome = chromosome;
        }

        public IReadOnlyCollection<Workshop> Workshops => Enumerable
            .Range(0, _inputData.Workshops.Count)
            .Where(w => _chromosome.Slot(w) == _id)
            .Select(w => new Workshop(w, _chromosome))
            .ToImmutableList();
    }
}