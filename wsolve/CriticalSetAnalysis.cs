using System;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class CriticalSetAnalysis
    {
        public static readonly TimeSpan ProgressInterval = TimeSpan.FromSeconds(3);
        
        private readonly HashSet<CriticalSet> _sets;

        public CriticalSetAnalysis(InputData inputData)
            : this(inputData, true) { }

        private CriticalSetAnalysis(InputData inputData, bool analyze)
        {
            InputData = inputData;
            _sets = new HashSet<CriticalSet>();

            if (analyze)
            {
                Analyze();
            }

            var eligiblePrefLevels = InputData.PreferenceLevels
                .Where(p => ForPreference(p).FirstOrDefault()?.Size >= InputData.Slots.Count)
                .ToList();

            int prefLevelMin = eligiblePrefLevels.Any() ? eligiblePrefLevels.Min() : 0;
            
            PreferenceBound = _sets.Any()
                ? prefLevelMin
                : InputData.MaxPreference;
        }

        public InputData InputData { get; }

        public IEnumerable<CriticalSet> AllSets => _sets.AsEnumerable();

        public int PreferenceBound { get; }

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
            bool retry = true;
            while (retry)
            {
                retry = false;
                foreach (CriticalSet set in inputSet)
                {
                    if (inputSet.Where(s => s != set).Any(other => predicate(set, other)))
                    {
                        retry = true;
                        inputSet.Remove(set);
                        break;
                    }
                }
            }
        }

        private void Analyze()
        {            
            DateTime nextOutput = DateTime.Now + ProgressInterval;
            var newSet = new List<int>();

            int[] prefLevels = InputData.PreferenceLevels.Reverse().ToArray();

            for (int i = 0; i < prefLevels.Length; i++)
            {
                int pref = prefLevels[i];
                for (int p = 0; p < InputData.Participants.Count; p++)
                {
                    if (DateTime.Now > nextOutput)
                    {
                        float progress = i / (float) prefLevels.Length +
                                         (1f / prefLevels.Length) * (p / (float) InputData.ParticipantCount);
                        Status.Info($"    {100*progress:0.00}% (pref. {pref}/{prefLevels.Length}, participant {p}/{InputData.ParticipantCount}); {_sets.Count} sets so far.");
                        nextOutput = DateTime.Now + ProgressInterval;
                    }
                    newSet.Clear();
                    int minCount = 0;

                    for (int w = 0; w < InputData.Workshops.Count; w++)
                    {
                        if (InputData.Participants[p].preferences[w] <= pref)
                        {
                            newSet.Add(w);
                            minCount += InputData.Workshops[w].min;
                        }
                    }

                    if (minCount > InputData.ParticipantCount * (InputData.SlotCount - 1))
                    {
                        // It is impossible that this critical set is not fulfilled.
                        continue;
                    }

                    var c = new CriticalSet(pref, newSet);
                    if (!_sets.AsParallel().Where(s => s != c).Any(other => c.IsCoveredBy(other)))
                    {
                        _sets.Add(new CriticalSet(pref, newSet));
                    }
                }
            }

            var setArray = _sets.ToArray();
            for (int i = 0; i < setArray.Length; i++)
            {
                CriticalSet set = setArray[i];
                if (DateTime.Now > nextOutput)
                {
                    float progress = i / (float) setArray.Length;
                    Status.Info(
                        $"    {100 * progress:0.00}% Simplifying ({i}/{setArray.Length}); {_sets.Count} sets remaining.");
                    nextOutput = DateTime.Now + ProgressInterval;
                }

                if (_sets.Where(s => s != set).Any(other => set.IsCoveredBy(other)))
                {
                    _sets.Remove(set);
                }
            }
        }
    }
}