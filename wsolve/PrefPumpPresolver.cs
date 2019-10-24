using System;
using System.Collections.Generic;
using System.Threading;

namespace WSolve
{
    public class PrefPumpPresolver
    {
        public delegate void ResultHandlerDelegate(Chromosome newChromosome, int preferenceLevel);

        public void Presolve(InputData inputData, IEnumerable<Solution> primalSolutionSource, IFitness fitness,
            CancellationToken ct, ResultHandlerDelegate resultHandler)
        {
            using (var init = primalSolutionSource.GetEnumerator())
            {
                while (init.MoveNext() && !ct.IsCancellationRequested)
                {
                    var c = Chromosome.FromOutput(inputData, init.Current);

                    if (Options.NoPrefPump)
                    {
                        resultHandler(c, c.MaxUsedPreference);
                        break;
                    }

                    for (var pref = inputData.MaxPreference; pref >= 0 && !ct.IsCancellationRequested; pref--)
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