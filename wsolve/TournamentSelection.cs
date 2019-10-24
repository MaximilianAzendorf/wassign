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
            var arr = list.ToArray();

            if (arr.Length == 0) yield break;

            if (arr.Length <= number)
                foreach (var a in arr)
                    yield return a;

            var fitness = arr.Select(level.System.Fitness.Evaluate).ToArray();

            var candidates = new List<int>(Enumerable.Range(0, arr.Length));

            while (number-- > 0)
            {
                var nsize = RNG.NextFloat() + Math.Floor(TournamentSize) > TournamentSize
                    ? (int) Math.Floor(TournamentSize)
                    : (int) Math.Ceiling(TournamentSize);

                if (candidates.Count == 0) yield break;

                var best = candidates[RNG.NextInt(0, candidates.Count)];

                for (var i = 1; i < nsize; i++)
                {
                    var next = candidates[RNG.NextInt(0, candidates.Count)];
                    if (fitness[best].CompareTo(fitness[next]) > 0) best = next;
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