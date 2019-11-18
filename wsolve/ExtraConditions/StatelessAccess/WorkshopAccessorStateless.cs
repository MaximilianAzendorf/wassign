using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public class WorkshopAccessorStateless : WorkshopAccessorBase
    {
        public WorkshopAccessorStateless(int id, CustomExtraConditionsBaseStateless @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public SlotAccessorStateless Slot => 
            new SlotAccessorStateless(Chromosome.Slot(_id), (CustomExtraConditionsBaseStateless)_base, Chromosome);
        
        public IReadOnlyCollection<ParticipantAccessorStateless> Conductors 
            => Chromosome.InputData.Workshops[_id].conductors
                .Select(n => new ParticipantAccessorStateless(n, (CustomExtraConditionsBaseStateless)_base, Chromosome))
                .ToImmutableList();
        
        public CollectionStateless<WorkshopAccessorStateless, ParticipantAccessorStateless> Participants
            => new CollectionStateless<WorkshopAccessorStateless, ParticipantAccessorStateless>(this);
    }
}