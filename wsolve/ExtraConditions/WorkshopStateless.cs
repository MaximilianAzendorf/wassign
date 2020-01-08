using System;
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

        public WorkshopStateless this[int idx]
        {
            get
            {
                if (idx < 1)
                {
                    throw new ArgumentException($"Invalid part index {idx}.");
                }
                int ws = Id;
                for(int i = 1; i < idx; i++)
                {
                    ws = InputData.Workshops[ws].continuation ??
                         throw new ArgumentException($"Workshop {Id} does not have a part {idx}.");
                }
                
                return new WorkshopStateless(ws, InputData);
            }
        }

        public FieldStateless<WorkshopStateless, SlotStateless> Slot => 
            new FieldStateless<WorkshopStateless, SlotStateless>(this);
        
        public CollectionStateless<WorkshopStateless, ParticipantStateless> Participants
            => new CollectionStateless<WorkshopStateless, ParticipantStateless>(this);
    }
}