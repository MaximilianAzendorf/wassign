using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions 
{
    public class SlotAccessor : SlotAccessorBase
    {
        public SlotAccessor(int id, CustomExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<WorkshopAccessor> Workshops => Enumerable
            .Range(0, Chromosome.InputData.Workshops.Count)
            .Where(w => Chromosome.Slot(w) == _id)
            .Select(w => new WorkshopAccessor(w, (CustomExtraConditionsBase)_base, Chromosome))
            .ToImmutableList();
    }
}