using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve
{
    public class InputData
    {
        public InputData(MutableInputData data)
        {
            Workshops = data.Workshops.ToImmutableList();
            Participants = data.Participants.ToImmutableList();
            Slots = data.Slots.ToImmutableList();
        }

        public IReadOnlyList<(string name, int[] conductors, int min, int max)> Workshops { get; }

        public IReadOnlyList<(string name, int[] preferences)> Participants { get; }

        public IReadOnlyList<string> Slots { get; }

        public int MaxPreference => Participants.Any() ? Participants.Max(p => p.preferences.Max()) : 0;

        public IEnumerable<int> PreferenceLevels =>
            Participants.SelectMany(p => p.preferences).Distinct().OrderBy(x => x);

        public int WorkshopCount => Workshops.Count;
        public int ParticipantCount => Participants.Count;
        public int SlotCount => Slots.Count;
    }
}