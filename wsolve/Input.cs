using System;
using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public class Input
    {
        public List<(string name, int conductor, int min, int max)> Workshops { get; private set; } = new List<(string name, int conductor, int min, int max)>();
        public List<(string name, int[] preferences)> Participants  { get; private set; } = new List<(string name, int[] preferences)>();
        public List<string> Slots { get; private set; } = new List<string>();

        public int MaxPreference => Participants.Max(p => p.preferences.Max());

        public Input()
        {
        }

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
                T[] n = new T[array.Length];

                for (int i = 0; i < array.Length; i++)
                {
                    n[i] = array[shuffle[i]];
                }
                
                Array.Copy(n, array, array.Length);
                return array;
            }
            
            Status.Info("Applying shuffle.");
            Random random = new Random(seed);

            int[] wShuffle = getShuffle(Workshops.Count, random);
            int[] pShuffle = getShuffle(Participants.Count, random);

            Workshops = applyShuffle(wShuffle, Workshops.ToArray()).ToList();
            Participants = applyShuffle(pShuffle, Participants.ToArray())
                .Select(x => (x.name, applyShuffle(wShuffle, x.preferences)))
                .ToList();
        }
    }
}