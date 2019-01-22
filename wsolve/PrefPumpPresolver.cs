using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;

namespace wsolve
{
    public class PrefPumpPresolver
    {
        public delegate void ResultHandlerDelegate(Chromosome newChromosome, int preferenceLevel);
        
        public void Presolve(Input input, IEnumerable<Output> primalSolutionSource, CancellationToken ct, ResultHandlerDelegate resultHandler)
        {
            using (IEnumerator<Output> init = primalSolutionSource.GetEnumerator())
            {
                while (init.MoveNext() && !ct.IsCancellationRequested)
                {
                    Chromosome c = Chromosome.FromOutput(input, init.Current);

                    for (int pref = input.MaxPreference; pref >= 0 && !ct.IsCancellationRequested; pref--)
                    {
                        var r = PrefPumpHeuristic.TryPump(c, pref, 8, TimeSpan.FromMilliseconds(1000));
                        if (r == PrefPumpResult.Fail || r == PrefPumpResult.Partial)
                        {
                            resultHandler(c, pref);
                            break;
                        }
                    }
                }
            }
        }
    }
}