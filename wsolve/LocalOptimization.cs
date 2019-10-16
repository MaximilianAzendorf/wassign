using System;
using System.Diagnostics;
using System.Linq;

namespace WSolve
{
    public static class LocalOptimization
    {
        
        public static Chromosome Apply(Chromosome chromosome, IFitness fitness, out int alterationCount)
        {
            int round = 0;

            int shifts = 0;
            int swaps = 0;
            int roundAlteration = 1;
            while (roundAlteration > 0)
            {
                Status.Info($"Starting Local Optimization Round {++round} ({shifts} shift(s) and {swaps} swap(s) so far).");
                roundAlteration = 0;
                var input = chromosome.InputData;
                int[] remainingSpace = Enumerable.Range(0, input.Workshops.Count)
                    .Select(w => input.Workshops[w].max - chromosome.CountParticipants(w)).ToArray();

                int[,] assignment = new int[input.Participants.Count, input.Slots.Count];
                int[,] slotMap = new int[input.Participants.Count, input.Slots.Count];

                for (int p = 0; p < input.Participants.Count; p++)
                {
                    for (int s = 0; s < input.Slots.Count; s++)
                    {
                        int w = chromosome.Workshop(p, s);
                        assignment[p, chromosome.Slot(w)] = w;
                        slotMap[p, chromosome.Slot(w)] = s;
                    }
                }

                for (int p = 0; p < input.Participants.Count; p++)
                {
                    for (int s = 0; s < input.Slots.Count; s++)
                    {
                        for (int w = 0; w < input.Workshops.Count; w++)
                        {
                            if (remainingSpace[w] <= 0) continue;
                            if (chromosome.Slot(w) != s) continue;
                            if (w == assignment[p, s]) continue;

                            if (input.Participants[p].preferences[w] <
                                input.Participants[p].preferences[assignment[p, s]])
                            {
                                chromosome.Workshop(p, slotMap[p, s]) = w;

                                if (!fitness.IsFeasible(chromosome))
                                {
                                    chromosome.Workshop(p, slotMap[p, s]) = assignment[p, s];
                                    continue;
                                }
                                
                                remainingSpace[assignment[p, s]]++;
                                remainingSpace[w]--;
                                assignment[p, s] = w;
                                roundAlteration++;
                                shifts += 1;
                            }
                        }
                    }
                }

                for (int p1 = 0; p1 < input.Participants.Count; p1++)
                {
                    for (int p2 = p1 + 1; p2 < input.Participants.Count; p2++)
                    {
                        for (int s = 0; s < input.Slots.Count; s++)
                        {
                            if (input.Participants[p1].preferences[assignment[p2, s]] <
                                input.Participants[p1].preferences[assignment[p1, s]]
                             && input.Participants[p2].preferences[assignment[p1, s]] <
                                input.Participants[p2].preferences[assignment[p2, s]])
                            {
                                chromosome.Workshop(p1, slotMap[p1, s]) = assignment[p2, s];
                                chromosome.Workshop(p2, slotMap[p2, s]) = assignment[p1, s];
                                
                                if (!fitness.IsFeasible(chromosome))
                                {
                                    chromosome.Workshop(p1, slotMap[p1, s]) = assignment[p1, s];
                                    chromosome.Workshop(p2, slotMap[p2, s]) = assignment[p2, s];
                                    continue;
                                }

                                int w1 = assignment[p1, s];
                                assignment[p1, s] = assignment[p2, s];
                                assignment[p2, s] = w1;
                                roundAlteration++;
                                swaps += 1;
                            }
                        }
                    }
                }
            }

            alterationCount = swaps + shifts;
            return chromosome;
        }
    }
}