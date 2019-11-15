using System;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class MutableInputData
    {
        public List<(string name, int[] conductors, int min, int max)> Workshops { get; private set; } =
            new List<(string name, int[] conductors, int min, int max)>();

        public List<(string name, int[] preferences)> Participants { get; private set; } =
            new List<(string name, int[] preferences)>();

        public List<string> Slots { get; } = new List<string>();

        public int MaxPreference => Participants.Any() ? Participants.Max(p => p.preferences.Max()) : 0;

        public IEnumerable<int> PreferenceLevels =>
            Participants.SelectMany(p => p.preferences).Distinct().OrderBy(x => x);

        public void Shuffle(int seed)
        {
            int[] getShuffle(int length, Random rnd)
            {
                int[] a = Enumerable.Range(0, length).ToArray();

                for (int i = length - 1; i >= 0; i--)
                {
                    int j = rnd.Next(i + 1);
                    (a[i], a[j]) = (a[j], a[i]);
                }

                return a;
            }

            T[] applyShuffle<T>(int[] shuffle, T[] array)
            {
                var shuffledArray = new T[array.Length];

                for (int i = 0; i < array.Length; i++)
                {
                    shuffledArray[i] = array[shuffle[i]];
                }

                return shuffledArray;
            }

            Status.Info("Applying shuffle.");
            var random = new Random(seed);

            int[] wShuffle = getShuffle(Workshops.Count, random);
            int[] pShuffle = getShuffle(Participants.Count, random);

            Workshops = applyShuffle(wShuffle, Workshops.ToArray())
                .Select(ws => (ws.name, ws.conductors.Select(c => Array.IndexOf(pShuffle, c)).ToArray(), ws.min,
                    ws.max))
                .ToList();

            Participants = applyShuffle(pShuffle, Participants.ToArray())
                .Select(x => (x.name, applyShuffle(wShuffle, x.preferences)))
                .ToList();

            for (int ws = 0; ws < Workshops.Count; ws++)
            {
                foreach (int c in Workshops[ws].conductors)
                {
                    if (Participants[c].preferences[ws] != 0)
                    {
                        Status.Warning("Pref " + Participants[c].preferences[ws]);
                    }
                }
            }
        }

        public InputData ToImmutableInputData()
        {
            return new InputData(this);
        }
    }
}