using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class Workshop : WorkshopBase
    {
        private readonly Chromosome _chromosome;
        
        public Workshop(int id, Chromosome chromosome)
            : base(id, chromosome.InputData)
        {
            _chromosome = chromosome;
        }
        
        public IReadOnlyCollection<Participant> Participants
        {
            get
            {
                var list = new List<Participant>();
                for (int p = 0; p < _inputData.Participants.Count; p++)
                {
                    for (int n = 0; n < _inputData.Slots.Count; n++)
                    {
                        if (_chromosome.Workshop(p, n) == _id)
                        {
                            list.Add(new Participant(p, _chromosome));
                        }
                    }
                }

                return list.ToImmutableList();
            }
        }

        public Slot Slot => new Slot(_chromosome.Slot(_id), _chromosome);
    }
}