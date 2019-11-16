using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class WorkshopAccessor : WorkshopAccessorBase
    {
        public WorkshopAccessor(int id, ExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<ParticipantAccessor> Conductors => _chromosome.InputData.Workshops[_id].conductors
            .Select(n => new ParticipantAccessor(n, _base, _chromosome))
            .ToImmutableList();
        
        public IReadOnlyCollection<ParticipantAccessor> Participants
        {
            get
            {
                var list = new List<ParticipantAccessor>();
                for (int p = 0; p < _chromosome.InputData.Participants.Count; p++)
                {
                    for (int n = 0; n < _chromosome.InputData.Slots.Count; n++)
                    {
                        if (_chromosome.Workshop(p, n) == _id)
                        {
                            list.Add(new ParticipantAccessor(p, _base, _chromosome));
                        }
                    }
                }

                return list.ToImmutableList();
            }
        }

        public SlotAccessor Slot => new SlotAccessor(_chromosome.Slot(_id), _base, _chromosome);
    }
}