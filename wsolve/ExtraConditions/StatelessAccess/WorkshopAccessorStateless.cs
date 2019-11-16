using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public class WorkshopAccessorStateless : WorkshopAccessorBase
    {
        public WorkshopAccessorStateless(int id, ExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public IReadOnlyCollection<ParticipantAccessorStateless> Conductors 
            => _chromosome.InputData.Workshops[_id].conductors
                .Select(n => new ParticipantAccessorStateless(n, _base, _chromosome))
                .ToImmutableList();
        
        public CollectionStateless<WorkshopAccessorStateless, ParticipantAccessorStateless> Participants
            => new CollectionStateless<WorkshopAccessorStateless, ParticipantAccessorStateless>(this);
    }
}