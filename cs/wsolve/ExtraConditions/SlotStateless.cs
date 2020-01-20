using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class SlotStateless : SlotBase
    {
        public SlotStateless(int id, InputData inputData)
            : base(id, inputData)
        {
        }

        public CollectionStateless<SlotStateless, WorkshopStateless> Events =>
            new CollectionStateless<SlotStateless, WorkshopStateless>(this);
    }
}