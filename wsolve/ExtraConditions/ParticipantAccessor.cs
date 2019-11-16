using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class ParticipantAccessor : ParticipantAccessorBase
    {
        public ParticipantAccessor(int id, ExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<WorkshopAccessor> Workshops => Enumerable.Range(0, _chromosome.InputData.Slots.Count)
            .Select(n => _chromosome.Workshop(_id, n))
            .Select(w => new WorkshopAccessor(w, _base, _chromosome))
            .ToImmutableList();

        public WorkshopAccessor WorkshopAt(SlotAccessor slot)
        {
            return Workshops.Single(w => w.Slot == slot);
        }

        public WorkshopAccessor WorkshopAt(string slotNameFragment)
        {
            return WorkshopAt(_base.Slot(slotNameFragment));
        }
    }
}