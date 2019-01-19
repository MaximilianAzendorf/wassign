using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace wsolve
{
    public static class PrefPumpPresolver
    {
        public static IEnumerable<Chromosome> Presolve(Input input, TimeSpan limit)
        {
            Status.Info("Generating initial generation by feasibility pump heuristic (greedy base solver).");
            DateTime lastOutput = DateTime.MinValue;
            ConcurrentBag<Chromosome>[] chromosomes = new ConcurrentBag<Chromosome>[input.MaxPreference+1];
            for(int i = 0; i < chromosomes.Length; i++) chromosomes[i] = new ConcurrentBag<Chromosome>();
            DateTime start = DateTime.Now;
            
            using (IEnumerator<Output> init = new GreedySolver().SolveIndefinitely(input, CancellationToken.None)
                .GetEnumerator())
            {
                while (DateTime.Now - start < limit)
                {
                    init.MoveNext();
                    Chromosome c = Chromosome.FromOutput(input, init.Current);
                    
                    for (int pref = input.MaxPreference; pref >= 0; pref--)
                    {
                        var r = PrefPumpHeuristic.TryPump(c, pref, 12, TimeSpan.FromMilliseconds(500));
                        if (r == PrefPumpResult.FAIL || r == PrefPumpResult.PARTIAL)
                        {
                            chromosomes[pref].Add(c);
                            break;
                        }
                    }

                    if (DateTime.Now - lastOutput > TimeSpan.FromSeconds(1))
                    {
                        Status.Info("buckets: " + string.Join(" ", chromosomes.Select(x => x.Count.ToString().PadLeft(5))));
                        lastOutput = DateTime.Now;
                    }
                }

                for (int pref = 0; pref <= input.MaxPreference; pref++)
                {
                    foreach (var c in chromosomes[pref])
                    {
                        yield return c;
                    }
                }
            }
        }
    }
}