using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class WorkshopStateless : WorkshopBase
    {
        public WorkshopStateless(int id, InputData inputData)
            : base(id, inputData)
        {
        }

        public FieldStateless<WorkshopStateless, SlotStateless> Slot => 
            new FieldStateless<WorkshopStateless, SlotStateless>(this);
        
        public CollectionStateless<WorkshopStateless, ParticipantStateless> Participants
            => new CollectionStateless<WorkshopStateless, ParticipantStateless>(this);
    }
}