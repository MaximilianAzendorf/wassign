using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public class ParticipantAccessorStateless : ParticipantAccessorBase
    {
        public ParticipantAccessorStateless(int id, ExtraConditionsBase @base, Chromosome chromosome)
            : base(id, @base, chromosome)
        {
        }

        public CollectionStateless<ParticipantAccessorStateless, WorkshopAccessorStateless> Workshops
            => new CollectionStateless<ParticipantAccessorStateless, WorkshopAccessorStateless>(this);
    }
}