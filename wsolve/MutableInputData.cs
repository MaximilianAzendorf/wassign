using System;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class MutableInputData
    {
        public List<(string name, int min, int max)> Workshops { get; private set; } =
            new List<(string name, int min, int max)>();

        public List<(string name, int[] preferences)> Participants { get; private set; } =
            new List<(string name, int[] preferences)>();

        public List<string> Slots { get; } = new List<string>();

        public List<string> Constraints { get; } = new List<string>();
        
        public string Filter { get; set;  }
        
        public List<(int participant, int workshop)> Conductors { get; private set; } = 
            new List<(int participant, int workshop)>();

        public int MaxPreference => Participants.Any() ? Participants.Max(p => p.preferences.Max()) : 0;

        public IEnumerable<int> PreferenceLevels =>
            Participants.SelectMany(p => p.preferences).Distinct().OrderBy(x => x);

        public InputData ToImmutableInputData()
        {
            return new InputData(this);
        }

        internal InputData ToImmutableInputDataDontCompile()
        {
            return new InputData(this, false);
        }
    }
}