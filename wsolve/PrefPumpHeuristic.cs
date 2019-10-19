using System;
using System.Diagnostics;
using System.Linq;

namespace WSolve
{
    public enum PrefPumpResult
    {
        Fail,
        Partial,
        Success
    }
    
    public static class PrefPumpHeuristic
    {
        public static PrefPumpResult TryPump(Chromosome chromosome, int preference, int maxDepth, TimeSpan timeout)
        {
#if DEBUG
            if (Debugger.IsAttached) timeout = TimeSpan.MaxValue;
#endif
            int[] partShuffle = Enumerable.Range(0, chromosome.InputData.Participants.Count).OrderBy(_ => RNG.NextInt())
                .ToArray();
            
            bool tryPumpRec(Chromosome c, int p, int s, int wOrig, int pref, bool[] visited, int depth, DateTime start)
            {
                int pw = c.Workshop(p, s);
                if (maxDepth >= 0 && depth > maxDepth) return false;
                if (visited[p]) return false;
                
                if (chromosome.InputData.Workshops[pw].conductors.Contains(p)) return false;
                
                for (int w = 0; w < chromosome.InputData.Workshops.Count; w++)
                {
                    if (w == pw) continue;
                    if (c.Slot(w) != s) continue;

                    if (chromosome.InputData.Participants[p].preferences[w] >= pref) continue;
                    
                    if (w == wOrig 
                        || (c.CountParticipants(w) < chromosome.InputData.Workshops[w].max
                            && c.CountParticipants(wOrig) > chromosome.InputData.Workshops[wOrig].min))
                    {
                        c.Workshop(p, s) = w;
                        return true;
                    }
                }

                visited[p] = true;
                for (int pswitchi = 0; pswitchi < chromosome.InputData.Participants.Count; pswitchi++)
                {
                    int pswitch = partShuffle[pswitchi];
                    if (DateTime.Now - start > timeout) return false;
                    
                    if (pswitch == p) continue;
                    
                    int wswitch = c.Workshop(pswitch, s);
                    if (chromosome.InputData.Workshops[wswitch].conductors.Contains(pswitch)) continue;
                    
                    int switchPref = chromosome.InputData.Participants[p].preferences[wswitch];
                    if (switchPref >= pref) continue;

                    if (tryPumpRec(c, pswitch, s, wOrig, pref, visited, depth + 1, start))
                    {
                        c.Workshop(p, s) = wswitch;
                        return true;
                    }
                }
                visited[p] = false;
                return false;
            }

            bool[] visitedMap = new bool[chromosome.InputData.Participants.Count];
            DateTime startTime = DateTime.Now;

            int successes = 0;
            int fails = 0;
            
            for (int pi = 0; pi < chromosome.InputData.Participants.Count; pi++)
            {
                int p = partShuffle[pi];
                for (int s = 0; s < chromosome.InputData.Slots.Count; s++)
                {
                    if (chromosome.InputData.Participants[p].preferences[chromosome.Workshop(p, s)] < preference) continue;
                    if (tryPumpRec(chromosome, p, s, chromosome.Workshop(p, s), preference, visitedMap, 0, startTime))
                    {
                        successes++;
                    }
                    else
                    {
                        fails++;
                    }
                }
            }
            
            return successes == 0 ? PrefPumpResult.Fail : fails == 0 ? PrefPumpResult.Success : PrefPumpResult.Partial;
        }
    }
}