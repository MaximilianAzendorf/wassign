using System;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class TournamentSelection : ISelection
    {
        public TournamentSelection(float size)
        {
            TournamentSize = size;
        }

        public float TournamentSize { get; }

        public IEnumerable<Chromosome> Select(int number, GaLevel level, IEnumerable<Chromosome> list)
        {
            Chromosome[] arr = list.ToArray();

            if (arr.Length == 0)
            {
                yield break;
            }

            if (arr.Length <= number)
            {
                foreach (Chromosome a in arr)
                {
                    yield return a;
                }
            }

            (float major, float minor)[] fitness = arr.Select(level.ParentSystem.Fitness.Evaluate).ToArray();

            var candidates = new List<int>(Enumerable.Range(0, arr.Length));

            while (number-- > 0)
            {
                int nsize = RNG.NextFloat() + Math.Floor(TournamentSize) > TournamentSize
                    ? (int) Math.Floor(TournamentSize)
                    : (int) Math.Ceiling(TournamentSize);

                if (candidates.Count == 0)
                {
                    yield break;
                }

                int best = candidates[RNG.NextInt(0, candidates.Count)];

                for (int i = 1; i < nsize; i++)
                {
                    int next = candidates[RNG.NextInt(0, candidates.Count)];
                    if (fitness[best].CompareTo(fitness[next]) > 0)
                    {
                        best = next;
                    }
                }

                yield return arr[best];
                candidates.Remove(best);
            }
        }

        public override string ToString()
        {
            return $"tournament({TournamentSize})";
        }
    }
}