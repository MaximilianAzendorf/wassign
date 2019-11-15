using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace WSolve
{
    public class GreedySolver : ISolver
    {
        public Solution Solve(InputData inputData)
        {
            return SolveIndefinitely(inputData, CriticalSetAnalysis.Empty(inputData), CancellationToken.None).First();
        }

        public IEnumerable<Solution> SolveIndefinitely(InputData inputData, CriticalSetAnalysis csAnalysis,
            CancellationToken ctoken)
        {
            var rootRnd = new Random(Options.Seed ?? 123);

            // We try to satisfy all critical sets until the timeout is reached. Then we discard the critical sets of
            // the lowest preference level and try again.
            //
            while (!ctoken.IsCancellationRequested)
            {
                int preferenceLimit = csAnalysis.PreferenceBound;

                List<int>[] slots = null;
                while (slots == null && !ctoken.IsCancellationRequested)
                {
                    CriticalSet[] sets = csAnalysis.ForPreference(preferenceLimit).ToArray();

                    TimeSpan timeout = TimeSpan.FromSeconds(Options.CriticalSetTimeoutSeconds);

                    slots = SolveAssignment(inputData, sets, timeout, rootRnd.Next(), ctoken);

                    if (slots == null)
                    {
                        try
                        {
                            preferenceLimit = inputData.PreferenceLevels.SkipWhile(p => p <= preferenceLimit).First();
                        }
                        catch (InvalidOperationException)
                        {
                            preferenceLimit = inputData.MaxPreference;
                        }
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
                    for (int w = 0; w < slots[s].Count; w++)
                    {
                        scheduling[slots[s][w]] = s;
                    }
                }

                var assignment = new int[inputData.Participants.Count, inputData.Slots.Count];
                var part = new int[inputData.Workshops.Count];

                for (int i = 0; i < assignment.GetLength(0); i++)
                {
                    for (int j = 0; j < assignment.GetLength(1); j++)
                    {
                        assignment[i, j] = -1;
                    }
                }

                // Workshop conductors have to be in their own workshop.
                //
                for (int i = 0; i < inputData.Workshops.Count; i++)
                {
                    foreach (int conductor in inputData.Workshops[i].conductors)
                    {
                        assignment[conductor, scheduling[i]] = i;
                        part[i]++;
                    }
                }

                var rnd = new Random(rootRnd.Next());

                // Just create a shuffled list of all (participant, slot)-pairs.
                //
                IOrderedEnumerable<(int p, int s)> indices = Enumerable.Range(0, inputData.Participants.Count)
                    .SelectMany(p => Enumerable.Range(0, inputData.Slots.Count).Select(s => (p, s)))
                    .OrderBy(x => rnd.Next());

                foreach ((int p, int s) in indices)
                {
                    if (ctoken.IsCancellationRequested)
                    {
                        yield break;
                    }

                    if (assignment[p, s] != -1)
                    {
                        continue;
                    }

                    // When limitIndex = 0, we search for workshops that don't have yet reached ther minimum participant
                    // number. At limitIndex = 1 we search for all workshops that don't have yet reached their maximum
                    // participant number.
                    //
                    for (int limitIndex = 0; limitIndex < 2; limitIndex++)
                    {
                        for (int w = 0; w < inputData.Workshops.Count; w++)
                        {
                            if (scheduling[w] != s)
                            {
                                continue;
                            }

                            int limit = limitIndex == 0 ? inputData.Workshops[w].min : inputData.Workshops[w].max;

                            if (part[w] < limit)
                            {
                                assignment[p, s] = w;
                                part[w]++;
                                goto assigned;
                            }
                        }
                    }

                    assigned: ;
                }

                var outputScheduling = new List<(int workshop, int slot)>();
                var outputAssignment = new List<(int participant, int workshop)>();

                for (int i = 0; i < inputData.Workshops.Count; i++)
                {
                    outputScheduling.Add((i, scheduling[i]));
                }

                for (int i = 0; i < inputData.Participants.Count; i++)
                {
                    for (int s = 0; s < inputData.Slots.Count; s++)
                    {
                        outputAssignment.Add((i, assignment[i, s]));
                    }
                }

                yield return new Solution(inputData, outputScheduling, outputAssignment);
            }
        }

        private List<int>[] SolveAssignment(InputData inputData, CriticalSet[] criticalSets, TimeSpan timeout, int seed,
            CancellationToken ctoken)
        {
            DateTime startTime = DateTime.Now;
            var rnd = new Random(seed);

            int[] workshopScramble = Enumerable.Range(0, inputData.Workshops.Count).OrderBy(x => rnd.Next()).ToArray();

            Stack<(int ws, int slot)> decisions = new Stack<(int, int)>();

            var backtracking = new Stack<List<int>>();

            for (int depth = 0; depth < workshopScramble.Length;)
            {
                if (ctoken.IsCancellationRequested)
                {
                    return null;
                }

                if (DateTime.Now > startTime + timeout)
                {
                    return null;
                }

                // no solution
                //
                if (depth == -1)
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
                                .Where(w => w.slot == s)
                                .Sum(w => inputData.Workshops[w.ws].max) + availableMaxPush <
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
                            // Feasible slots are all slots for which adding the current workshop would not cause the
                            // minimal participant number of this slot to exceed the total participant count.
                            //
                            // We order the feasible slots by the maximal participant number as a heuristic to get more
                            // balanced schedulings.
                            //
                            int[] feasibleSlots = Enumerable.Range(0, inputData.Slots.Count)
                                .Where(s =>
                                    decisions
                                        .Where(w => w.slot == s)
                                        .Sum(w => inputData.Workshops[w.ws].min) + inputData.Workshops[workshop].min <=
                                    inputData.Participants.Count)
                                .OrderBy(s =>
                                    decisions
                                        .Where(w => w.slot == s)
                                        .Sum(w => inputData.Workshops[w.ws].max))
                                .ToArray();

                            backtracking.Push(feasibleSlots.ToList());
                        }
                    }
                }

                if (!backtracking.Peek().Any())
                {
                    backtracking.Pop();
                    decisions.Pop();
                    depth--; // backtrack
                    continue;
                }

                int nextSlot = backtracking.Peek().First();
                backtracking.Peek().Remove(nextSlot);
                decisions.Push((workshop, nextSlot));
                depth++;
            }

            var slots = new List<int>[inputData.Slots.Count];
            for (int i = 0; i < slots.Length; i++)
            {
                slots[i] = new List<int>();
            }

            foreach ((int ws, int slot) decision in decisions)
            {
                slots[decision.slot].Add(decision.ws);
            }

            return slots;
        }

        private bool SatisfiesCriticalSets(InputData inputData, Stack<(int ws, int slot)> decisions,
            IEnumerable<CriticalSet> criticalSets)
        {
            Dictionary<int, int> decisionMap = decisions.ToDictionary(d => d.ws, d => d.slot);

            foreach (CriticalSet set in criticalSets)
            {
                var coveredSlots = new HashSet<int>();
                int missing = 0;

                foreach (int element in set)
                {
                    if (!decisionMap.TryGetValue(element, out int slot))
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