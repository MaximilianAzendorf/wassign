using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.CompilerServices;

namespace wsolve
{
    public class Output
    {
        public readonly IReadOnlyDictionary<int, int> Scheduling;
        public readonly IReadOnlyDictionary<int, IReadOnlyList<int>> Assignment;

        public IEnumerable<(int participant, int workshop)> FlatAssignment =>
            Assignment.SelectMany(kvp => kvp.Value.Select(x => (kvp.Key, x)));

        public Output(IEnumerable<(int workshop, int slot)> scheduling, IEnumerable<(int participant, int workshop)> assignment)
        {
            Scheduling = new ReadOnlyDictionary<int, int>(new Dictionary<int, int>(scheduling.Select(x => new KeyValuePair<int, int>(x.workshop, x.slot))));
            Assignment = new ReadOnlyDictionary<int, IReadOnlyList<int>>(
                new Dictionary<int, IReadOnlyList<int>>(assignment.GroupBy(kvp => kvp.participant).Select(x =>
                    new KeyValuePair<int, IReadOnlyList<int>>(x.Key,
                        x.Select(w => w.workshop).ToList().AsReadOnly()))));
        }

        public void Verify(Input input)
        {
            void workshopMinConstraint(int w)
            {
                if(FlatAssignment.Count(kvp => kvp.workshop == w) < input.Workshops[w].min)
                    throw new VerifyException($"Workshop '{input.Workshops[w].name}' has too few participants.");
            }
            
            void workshopMaxConstraint(int w)
            {
                if(FlatAssignment.Count(kvp => kvp.workshop == w) > input.Workshops[w].max)
                    throw new VerifyException($"Workshop '{input.Workshops[w].name}' has too many participants.");
            }

            void oneWorkshopPerSlot(int p, int s)
            {
                int c = FlatAssignment.Count(kvp => kvp.participant == p && Scheduling[kvp.workshop] == s);
                if (c < 1)
                    throw new VerifyException($"Participant '{input.Participants[p].name}' has no workshop in slot '{input.Slots[s]}'.");
                if (c > 1)
                    throw new VerifyException($"Participant '{input.Participants[p].name}' has more than one workshop in slot '{input.Slots[s]}'.");
            }

            void conductorIsInOwnWorkshop(int w)
            {
                if(!Assignment[input.Workshops[w].conductor].Contains(w))
                    throw new VerifyException($"Participant '{input.Participants[input.Workshops[w].conductor]}', conductor of '{input.Workshops[w].name}', is not in his own workshop.");
            }

            for (int w = 0; w < input.Workshops.Count; w++)
            {
                workshopMinConstraint(w);
                workshopMaxConstraint(w);
                conductorIsInOwnWorkshop(w);
            }

            for (int p = 0; p < input.Participants.Count; p++)
            {
                for (int s = 0; s < input.Slots.Count; s++)
                {
                    oneWorkshopPerSlot(p, s);
                }
            }
        }
    }
}