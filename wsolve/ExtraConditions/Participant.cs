using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions
{
    public class Participant : ParticipantBase
    {
        private readonly Chromosome _chromosome;
        
        public Participant(int id, Chromosome chromosome)
            : base(id, chromosome.InputData)
        {
            _chromosome = chromosome;
        }

        public IReadOnlyCollection<Workshop> Workshops => Enumerable.Range(0, _inputData.Slots.Count)
            .Select(n => _chromosome.Workshop(_id, n))
            .Select(w => new Workshop(w, _chromosome))
            .ToImmutableList();

        public Workshop WorkshopAt(Slot slot)
        {
            return Workshops.Single(w => w.Slot == slot);
        }
    }
}