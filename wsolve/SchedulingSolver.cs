using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using System.Threading;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.Constraints;

namespace WSolve
{
    public static class SchedulingSolver
    {
        private static readonly Random SeedSource = new Random();
        private const int PREF_RELAXATION = 10;
        
        public static Scheduling Solve(InputData inputData)
        {
            return SolveIndefinitely(inputData, CriticalSetAnalysis.Empty(inputData), CancellationToken.None).FirstOrDefault();
        }

        public static IEnumerable<Scheduling> SolveIndefinitely(InputData inputData, CriticalSetAnalysis csAnalysis,
            CancellationToken ctoken)
        {
            Random rootRnd;
            lock (SeedSource)
            {
                rootRnd = new Random(SeedSource.Next());
            }

            while (!ctoken.IsCancellationRequested)
            {
                int preferenceLimit = rootRnd.Next(PREF_RELAXATION) == 0 ? inputData.MaxPreference : csAnalysis.PreferenceBound;

                int[][] slots = null;
                
                // We try to satisfy all critical sets until the timeout is reached. Then we discard the critical sets of
                // the lowest preference level and try again.
                //
                while (slots == null && !ctoken.IsCancellationRequested)
                {
                    CriticalSet[] sets = csAnalysis.ForPreference(preferenceLimit).ToArray();

                    DateTime timeLimit = preferenceLimit == inputData.MaxPreference ? DateTime.MaxValue : DateTime.Now + TimeSpan.FromSeconds(Options.CriticalSetTimeoutSeconds);

                    slots = SolveScheduling(inputData, sets, timeLimit, rootRnd.Next(), ctoken);

                    if (slots == null)
                    {
                        if (preferenceLimit == inputData.MaxPreference)
                        {
                            yield break;
                        }
                        preferenceLimit = inputData.PreferenceLevels.SkipWhile(p => p <= preferenceLimit).First();
                    }
                }

                if (ctoken.IsCancellationRequested)
                {
                    yield break;
                }

                // Just copy the scheduling solution to a flat array for better access.
                //
                var scheduling = new int[inputData.Workshops.Count];

                for (int s = 0; s < slots.Length; s++)
                {
                    for (int w = 0; w < slots[s].Length; w++)
                    {
                        scheduling[slots[s][w]] = s;
                    }
                }

                var outputScheduling = new List<(int workshop, int slot)>();

                for (int i = 0; i < inputData.Workshops.Count; i++)
                {
                    outputScheduling.Add((i, scheduling[i]));
                }

                yield return new Scheduling(inputData, outputScheduling);
            }
        }

        
        private static int[][] SolveScheduling(InputData inputData, CriticalSet[] criticalSets, DateTime timeLimit, 
            int seed, CancellationToken ctoken)
        {
            DateTime startTime = DateTime.Now;
            var rnd = new Random(seed);

            int[] workshopScramble = Enumerable.Range(0, inputData.Workshops.Count)
                .OrderBy(x => -inputData.GetSchedulingConstraintsForWorkshop(x).Count())
                .ThenBy(x => rnd.Next())
                .ToArray();

            bool[] lowPrioritySlot = Enumerable.Range(0, inputData.SlotCount)
                .Select(s => inputData.Slots[s].StartsWith(InputData.NotScheduledSlotPrefix)).ToArray();
            
            Dictionary<int, int> decisions = new Dictionary<int, int>();

            var backtracking = new Stack<List<int>>();

            for (int depth = 0; depth < workshopScramble.Length;)
            {
                if (ctoken.IsCancellationRequested)
                {
                    return null;
                }

                if (DateTime.Now > timeLimit)
                {
                    return null;
                }

                int workshop = workshopScramble[depth];

                if (backtracking.Count <= depth)
                {
                    // The maximum number of participants that can be covered with all workshops that are not yet assigned
                    // to a slot.
                    //
                    int availableMaxPush = workshopScramble.Skip(depth).Sum(w => inputData.Workshops[w].max);

                    // Impossibilities are slots that contain so few workshops that even with all the workshops not yet
                    // assigned they would not have enough capacity for all participants.
                    //
                    IEnumerable<int> impossibilities = Enumerable.Range(0, inputData.Slots.Count)
                        .Where(s =>
                            decisions
                                .Where(w => w.Value == s)
                                .Sum(w => inputData.Workshops[w.Key].max) + availableMaxPush <
                            inputData.Participants.Count);

                    // If there are any impossibilities, the current partial solution is infeasible.
                    //
                    if (impossibilities.Any())
                    {
                        backtracking.Push(new List<int>());
                    }
                    else
                    {
                        // If the partial solution does not satisfy critical set constraints it is infeasible.
                        //
                        // This is the case when there aren't enough elements in a critical set to cover all slots. For
                        // example, for 4 Slots and the critical set {A, B, C, D}, a partial solution of the form
                        //
                        //      Slot 1:    ... A, C ....
                        //      Slot 2:    ... D .......
                        //      Slot 3:    .............
                        //      Slot 4:    .............
                        //   Not assigned: ... B .......
                        //
                        // Would not be feasible, because the critical set can not be covered anymore (we would need at
                        // least 2 open workshops in the critical set to cover Slot 3 and 4).
                        //
                        if (!SatisfiesCriticalSets(inputData, decisions, criticalSets))
                        {
                            backtracking.Push(new List<int>());
                        }
                        else
                        {
                            // Critical slots are slots that need the current workshop in order to still be able
                            // to fulfill the participant count.
                            //
                            var criticalSlots = Enumerable.Range(0, inputData.Slots.Count)
                                .OrderBy(x => RNG.NextInt())
                                .Where(s =>
                                    decisions
                                        .Where(w => w.Value == s)
                                        .Sum(w => inputData.Workshops[w.Key].max) + availableMaxPush -
                                    inputData.Workshops[workshop].max <
                                    inputData.ParticipantCount)
                                .Where(s => SatisfiesSchedulingConstraints(workshop, s, decisions, inputData))
                                .ToList();

                            if (criticalSlots.Count == 1)
                            {
                                backtracking.Push(criticalSlots);
                            }
                            else if (criticalSlots.Count > 1)
                            {
                                backtracking.Push(new List<int>());
                            }
                            else
                            {
                                // Feasible slots are all slots for which adding the current workshop would not cause the
                                // minimal participant number of this slot to exceed the total participant count.
                                //
                                // We then have to filter the feasible slot by all additional constraints.
                                //
                                // We order the feasible slots by the maximal participant number as a heuristic to get more
                                // balanced schedulings.
                                //
                                var feasibleSlots = Enumerable.Range(0, inputData.Slots.Count)
                                    .Where(s =>
                                        decisions
                                            .Where(w => w.Value == s)
                                            .Sum(w => inputData.Workshops[w.Key].min) +
                                        inputData.Workshops[workshop].min <=
                                        inputData.Participants.Count)
                                    .Where(s => SatisfiesSchedulingConstraints(workshop, s, decisions, inputData))
                                    .ToList();

                                var orderedSlots = feasibleSlots
                                    .Where(s => !lowPrioritySlot[s])
                                    .OrderBy(s =>
                                        decisions
                                            .Where(w => w.Value == s)
                                            .Sum(w => inputData.Workshops[w.Key].max))
                                    .ToList();
                                
                                foreach(var low in feasibleSlots.Where(s => lowPrioritySlot[s]))
                                {
                                    orderedSlots.Insert(RNG.NextInt(0, orderedSlots.Count + 1), low);
                                }

                                backtracking.Push(orderedSlots);
                            }
                        }
                    }
                }

                if (!backtracking.Peek().Any())
                {
                    // no solution
                    //
                    if (depth == 0)
                    {
                        return null;
                    }
                    
                    backtracking.Pop();
                    decisions.Remove(workshopScramble[depth - 1]);
                    depth--; // backtrack
                    continue;
                }

                int nextSlot = backtracking.Peek().First();
                backtracking.Peek().Remove(nextSlot);
                decisions.Add(workshop, nextSlot);
                depth++;
            }

            var slots = new List<int>[inputData.Slots.Count];
            for (int i = 0; i < slots.Length; i++)
            {
                slots[i] = new List<int>();
            }

            foreach (var decision in decisions)
            {
                slots[decision.Value].Add(decision.Key);
            }

            return slots.Select(x => x.ToArray()).ToArray();
        }
        
        private static bool SatisfiesSchedulingConstraints(int workshop, int slot, Dictionary<int, int> decisions, 
            InputData inputData)
        {
            foreach (var constraint in inputData.GetSchedulingConstraintsForWorkshop(workshop))
            {
                switch (constraint)
                {
                    case SetValueConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        if(slot != c.Value.Id) return false;
                        break;
                    }
                    case ForbidValueConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        if(slot == c.Value.Id) return false;
                        break;
                    }
                    case EqualsConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        int other = c.Left.Id == workshop ? c.Right.Id : c.Left.Id;
                        if(decisions.TryGetValue(other, out var otherSlot) && otherSlot != slot) return false;
                        break;
                    }
                    case EqualsNotConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        int other = c.Left.Id == workshop ? c.Right.Id : c.Left.Id;
                        if(decisions.TryGetValue(other, out var otherSlot) && otherSlot == slot) return false;
                        break;
                    }
                    case SlotOffsetConstraint c:
                    {
                        int other = c.First.Id == workshop ? c.Second.Id : c.First.Id;
                        int offset = other == c.First.Id ? -c.Offset : c.Offset;
                        if(decisions.TryGetValue(other, out var otherSlot) && otherSlot - slot != offset) return false;
                        
                        int minSlot = Math.Max(0, 0 - offset);
                        int maxSlot = Math.Min(inputData.SlotCount, inputData.SlotCount - offset);
                        
                        if(slot < minSlot || slot >= maxSlot) return false;
                        break;
                    }
                    default:
                    {
                        throw new ArgumentException($"Unknown constraint type {constraint}.");
                    }
                }
            }
            
            return true;
        }

        private static bool SatisfiesCriticalSets(InputData inputData, Dictionary<int, int> decisions,
            IEnumerable<CriticalSet> criticalSets)
        {
            var coveredSlots = new HashSet<int>();
            foreach (CriticalSet set in criticalSets)
            {
                coveredSlots.Clear();
                int missing = 0;

                foreach (int element in set)
                {
                    if (!decisions.TryGetValue(element, out int slot))
                    {
                        missing++;
                    }
                    else
                    {
                        coveredSlots.Add(slot);
                    }
                }

                if (coveredSlots.Count + missing < inputData.Slots.Count)
                {
                    return false;
                }
            }

            return true;
        }
    }
}