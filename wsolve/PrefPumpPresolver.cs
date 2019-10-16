using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;

namespace WSolve
{
    public class PrefPumpPresolver
    {
        public delegate void ResultHandlerDelegate(Chromosome newChromosome, int preferenceLevel);
        
        public void Presolve(InputData inputData, IEnumerable<Solution> primalSolutionSource, CancellationToken ct, ResultHandlerDelegate resultHandler)
        {
            using (IEnumerator<Solution> init = primalSolutionSource.GetEnumerator())
            {
                while (init.MoveNext() && !ct.IsCancellationRequested)
                {
                    Chromosome c = Chromosome.FromOutput(inputData, init.Current);

                    for (int pref = inputData.MaxPreference; pref >= 0 && !ct.IsCancellationRequested; pref--)
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