using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class WorkshopAccessor : WorkshopAccessorBase
    {
        public WorkshopAccessor(int id, CustomExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<ParticipantAccessor> Conductors => Chromosome.InputData.Workshops[_id].conductors
            .Select(n => new ParticipantAccessor(n, (CustomExtraConditionsBase)_base, Chromosome))
            .ToImmutableList();
        
        public IReadOnlyCollection<ParticipantAccessor> Participants
        {
            get
            {
                var list = new List<ParticipantAccessor>();
                for (int p = 0; p < Chromosome.InputData.Participants.Count; p++)
                {
                    for (int n = 0; n < Chromosome.InputData.Slots.Count; n++)
                    {
                        if (Chromosome.Workshop(p, n) == _id)
                        {
                            list.Add(new ParticipantAccessor(p, (CustomExtraConditionsBase)_base, Chromosome));
                        }
                    }
                }

                return list.ToImmutableList();
            }
        }

        public SlotAccessor Slot => new SlotAccessor(Chromosome.Slot(_id), (CustomExtraConditionsBase)_base, Chromosome);
    }
}