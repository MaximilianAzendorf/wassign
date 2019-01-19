using System;
using System.Diagnostics;

namespace wsolve
{
    public enum PrefPumpResult
    {
        FAIL,
        PARTIAL,
        SUCCESS
    }
    
    public static class PrefPumpHeuristic
    {
        public static PrefPumpResult TryPump(Chromosome chromosome, int preference, int maxDepth, TimeSpan timeout)
        {
#if DEBUG
            if (Debugger.IsAttached) timeout = TimeSpan.MaxValue;
#endif
            
            bool tryPumpRec(Chromosome c, int p, int s, int wOrig, int pref, bool[] visited, int depth, DateTime start)
            {
                if (depth > maxDepth) return false;
                if (chromosome.Input.Workshops[c.Workshop(p, s)].conductor == p) return false;
                
                for (int w = 0; w < chromosome.Input.Workshops.Count; w++)
                {
                    if (c.Slot(w) != s) continue;

                    if (chromosome.Input.Participants[p].preferences[w] >= pref) continue;
                    
                    if (c.CountParticipants(w) < chromosome.Input.Workshops[w].max + (w == wOrig ? 1 : 0))
                    {
                        c.Workshop(p, s) = w;
                        return true;
                    }
                }

                visited[p] = true;
                for (int pswitch = 0; pswitch < chromosome.Input.Participants.Count; pswitch++)
                {
                    if (DateTime.Now - start > timeout) return false;
                    
                    if (pswitch == p) continue;
                    
                    int wswitch = c.Workshop(pswitch, s);
                    if (chromosome.Input.Workshops[wswitch].conductor == pswitch) continue;
                    
                    int switchPref = chromosome.Input.Participants[p].preferences[wswitch];
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

            bool[] visitedMap = new bool[chromosome.Input.Participants.Count];
            DateTime startTime = DateTime.Now;

            int successes = 0;
            int fails = 0;
            for (int p = 0; p < chromosome.Input.Participants.Count; p++)
            {
                for (int s = 0; s < chromosome.Input.Slots.Count; s++)
                {
                    if (chromosome.Input.Participants[p].preferences[chromosome.Workshop(p, s)] < preference) continue;
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

            return successes == 0 ? PrefPumpResult.FAIL : fails == 0 ? PrefPumpResult.SUCCESS : PrefPumpResult.PARTIAL;
        }
    }
}