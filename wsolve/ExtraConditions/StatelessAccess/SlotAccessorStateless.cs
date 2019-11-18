using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public class SlotAccessorStateless : SlotAccessorBase
    {
        public SlotAccessorStateless(int id, CustomExtraConditionsBaseStateless @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<WorkshopAccessorStateless> Workshops => Enumerable
            .Range(0, Chromosome.InputData.Workshops.Count)
            .Where(w => Chromosome.Slot(w) == _id)
            .Select(w => new WorkshopAccessorStateless(w, (CustomExtraConditionsBaseStateless)_base, Chromosome))
            .ToImmutableList();
    }
}