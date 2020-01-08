using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class ParticipantStateless : ParticipantBase
    {
        public ParticipantStateless(int id, InputData inputData)
            : base(id, inputData)
        {
        }

        public CollectionStateless<ParticipantStateless, WorkshopStateless> Events
            => new CollectionStateless<ParticipantStateless, WorkshopStateless>(this);
    }
}