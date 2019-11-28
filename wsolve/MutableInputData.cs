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
                .ToList();

            Participants = applyShuffle(pShuffle, Participants.ToArray())
                .Select(x => (x.name, applyShuffle(wShuffle, x.preferences)))
                .ToList();

            Conductors = Conductors
                .Select(x => (Array.IndexOf(pShuffle, x.participant), Array.IndexOf(wShuffle, x.workshop)))
                .ToList();
        }

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