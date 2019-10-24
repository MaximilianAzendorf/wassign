using System;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class CriticalSetAnalysis
    {
        private readonly HashSet<CriticalSet> _sets;

        public CriticalSetAnalysis(InputData inputData)
            : this(inputData, true)
        {
        }

        private CriticalSetAnalysis(InputData inputData, bool analyze)
        {
            InputData = inputData;
            _sets = new HashSet<CriticalSet>();

            if (analyze) Analyze();
        }

        public InputData InputData { get; }

        public IEnumerable<CriticalSet> AllSets => _sets.AsEnumerable();

        public int PreferenceBound => _sets.Any()
            ? InputData.PreferenceLevels.Where(p => ForPreference(p).First().Size >= InputData.Slots.Count).Min()
            : InputData.MaxPreference;

        public static CriticalSetAnalysis Empty(InputData inputData)
        {
            return new CriticalSetAnalysis(inputData, false);
        }

        public IEnumerable<CriticalSet> ForPreference(int preference)
        {
            var prefSets = new HashSet<CriticalSet>(AllSets.Where(s => s.Preference >= preference));

            ThinOutBy(prefSets, (set, other) => other.IsSubsetOf(set));

            return prefSets.OrderBy(p => p.Size).ToArray();
        }

        private static void ThinOutBy(HashSet<CriticalSet> inputSet, Func<CriticalSet, CriticalSet, bool> predicate)
        {
            var retry = true;
            while (retry)
            {
                retry = false;
                foreach (var set in inputSet)
                    if (inputSet.Where(s => s != set).Any(other => predicate(set, other)))
                    {
                        retry = true;
                        inputSet.Remove(set);
                        break;
                    }
            }
        }

        private void Analyze()
        {
            var newSet = new List<int>();

            foreach (var pref in InputData.PreferenceLevels.Reverse())
                for (var p = 0; p < InputData.Participants.Count; p++)
                {
                    newSet.Clear();

                    for (var w = 0; w < InputData.Workshops.Count; w++)
                        if (InputData.Participants[p].preferences[w] <= pref)
                            newSet.Add(w);

                    var c = new CriticalSet(pref, newSet);
                    if (!_sets.Where(s => s != c).Any(other => c.IsCoveredBy(other)))
                        _sets.Add(new CriticalSet(pref, newSet));
                }

            Simplify();
        }

        private void Simplify()
        {
            ThinOutBy(_sets, (set, other) => set.IsCoveredBy(other));
        }
    }
}