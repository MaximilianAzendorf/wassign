using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class ParticipantAccessor : ParticipantAccessorBase
    {
        public ParticipantAccessor(int id, CustomExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<WorkshopAccessor> Workshops => Enumerable.Range(0, Chromosome.InputData.Slots.Count)
            .Select(n => Chromosome.Workshop(_id, n))
            .Select(w => new WorkshopAccessor(w, (CustomExtraConditionsBase)_base, Chromosome))
            .ToImmutableList();

        public WorkshopAccessor WorkshopAt(SlotAccessor slot)
        {
            return Workshops.Single(w => w.Slot == slot);
        }
    }
}