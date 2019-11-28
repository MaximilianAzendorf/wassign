using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using System.Threading;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.Constraints;

namespace WSolve
{
    public class GreedySolver : ISolver
    {
        private static readonly Random SeedSource = new Random(125);
        private const int PREF_RELAXATION = 10;
        
        public Solution Solve(InputData inputData)
        {
            return SolveIndefinitely(inputData, CriticalSetAnalysis.Empty(inputData), CancellationToken.None, false).First();
        }

        public Scheduling SolveSchedulingOnly(InputData inputData)
        {
            return SolveIndefinitelySchedulingOnly(inputData, CriticalSetAnalysis.Empty(inputData), CancellationToken.None).First();
        }

        public IEnumerable<Solution> SolveIndefinitely(InputData inputData, CriticalSetAnalysis csAnalysis,
            CancellationToken ctoken)
        {
            return SolveIndefinitely(inputData, csAnalysis, ctoken, false);
        }
        
        public IEnumerable<Scheduling> SolveIndefinitelySchedulingOnly(InputData inputData, CriticalSetAnalysis csAnalysis,
            CancellationToken ctoken)
        {
            return SolveIndefinitely(inputData, csAnalysis, ctoken, true).Select(s => new Scheduling(inputData, s.Scheduling));
        }

        private IEnumerable<Solution> SolveIndefinitely(InputData inputData, CriticalSetAnalysis csAnalysis,
            CancellationToken ctoken, bool schedulingOnly)
        {
            Random rootRnd;
            lock (SeedSource)
            {
                rootRnd = new Random(Options.Seed ?? SeedSource.Next());
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

                if (schedulingOnly)
                {
                    yield return new Solution(inputData, outputScheduling, null);
                }
                else
                {
                    var assignment = SolveAssignment(inputData, scheduling, ctoken, rootRnd.Next());

                    // This scheduling seems to have no solution
                    if (assignment == null)
                    {
                        continue;
                    }
                    
                    var outputAssignment = new List<(int participant, int workshop)>();

                    for (int p = 0; p < inputData.Participants.Count; p++)
                    {
                        for (int s = 0; s < inputData.Slots.Count; s++)
                        {
                            outputAssignment.Add((p, assignment[p][s]));
                        }
                    }

                    yield return new Solution(inputData, outputScheduling, outputAssignment);
                }
            }
        }

        private Dictionary<int, int[]> SolveAssignment(InputData inputData, int[] scheduling, CancellationToken ctoken, int seed)
        {
            Dictionary<int, int[]> solution = Enumerable.Range(0, inputData.ParticipantCount)
                .ToDictionary(p => p, _ => new int[inputData.SlotCount]);

            foreach (var array in solution.Values)
            {
                Array.Fill(array, -1);
            }

            if (!SolveAssignmentForSlot(inputData, scheduling, 0, ctoken, solution, seed))
            {
                return null;
            }

            return solution;
        }
        
        private bool SolveAssignmentForSlot(InputData inputData, int[] scheduling, int slot, CancellationToken ctoken, Dictionary<int, int[]> fullDecisions, int seed)
        {
            float backtrackChance = (float)Math.Min(0.999f, Math.Pow(2, -2f / inputData.ParticipantCount));
            
            var rnd = new Random(seed);
            
            Dictionary<int, int> decisions = new Dictionary<int, int>();
            Stack<List<int>> backtracking = new Stack<List<int>>();
            int[] partCount = new int[inputData.WorkshopCount];

            int[] participantScramble = Enumerable.Range(0, inputData.ParticipantCount)
                .OrderBy(x => -inputData.GetAssignmentConstraintsForParticipant(x).Count())
                .ThenBy(x => rnd.Next())
                .ToArray();

            int[] workshops = Enumerable.Range(0, inputData.WorkshopCount).Where(w => scheduling[w] == slot).ToArray();
            
            for (int depth = 0; depth <= participantScramble.Length;)
            {
                int participant = depth < participantScramble.Length ? participantScramble[depth] : -1;
                
                if (depth < participantScramble.Length)
                {
                    if (ctoken.IsCancellationRequested)
                    {
                        return false;
                    }

                    if (backtracking.Count <= depth)
                    {
                        List<int> feasibleWorkshops = new List<int>();
                        Dictionary<int, int> deficits = new Dictionary<int, int>();

                        foreach (var workshop in workshops.OrderBy(_ => RNG.NextInt()))
                        {
                            if (partCount[workshop] >= inputData.Workshops[workshop].max)
                            {
                                continue;
                            }

                            int deficit = inputData.Workshops[workshop].min - partCount[workshop];

                            if (deficit > 0)
                            {
                                deficits.Add(workshop, deficit);
                            }

                            feasibleWorkshops.Add(workshop);
                        }

                        if (deficits.Values.Sum() == (inputData.ParticipantCount - depth))
                        {
                            feasibleWorkshops = feasibleWorkshops.Intersect(deficits.Keys).ToList();
                        }

                        feasibleWorkshops.RemoveAll(w =>
                            !SatisfiesAssignmentConstraints(participant, w, slot, scheduling, fullDecisions,
                                inputData));

                        backtracking.Push(feasibleWorkshops);
                    }
                }
                else
                {
                    if (slot == inputData.SlotCount - 1)
                    {
                        return true;
                    }
                    else
                    {
                        if (SolveAssignmentForSlot(inputData, scheduling, slot + 1, ctoken, fullDecisions,
                            rnd.Next()))
                        {
                            return true;
                        }
                        else
                        {
                            backtracking.Push(new List<int>());
                        }
                    }
                }

                if (!backtracking.Peek().Any())
                {
                    // we go multiple levels up so we can find a solution faster.
                    //
                    while (RNG.NextFloat() < backtrackChance)
                    {
                        // no solution
                        //
                        if (depth == 0)
                        {
                            return false;
                        }

                        backtracking.Pop();
                        int ws = fullDecisions[participantScramble[depth - 1]][slot];
                        fullDecisions[participantScramble[depth - 1]][slot] = -1;
                        decisions.Remove(participantScramble[depth - 1]);
                        partCount[ws]--;
                        depth--; // backtrack
                    }

                    continue;
                }

                int nextWorkshop = backtracking.Peek().First();
                backtracking.Peek().Remove(nextWorkshop);
                decisions.Add(participant, nextWorkshop);
                fullDecisions[participant][slot] = nextWorkshop;
                partCount[nextWorkshop]++;
                depth++;
            }
            
            return true;
        }

        private int[][] SolveScheduling(InputData inputData, CriticalSet[] criticalSets, DateTime timeLimit, int seed,
            CancellationToken ctoken)
        {
            DateTime startTime = DateTime.Now;
            var rnd = new Random(seed);

            int[] workshopScramble = Enumerable.Range(0, inputData.Workshops.Count)
                .OrderBy(x => -inputData.GetSchedulingConstraintsForWorkshop(x).Count())
                .ThenBy(x => rnd.Next())
                .ToArray();
            
            Dictionary<int, int> decisions = new Dictionary<int, int>();

            var backtracking = new Stack<List<int>>();

            for (int depth = 0; depth < workshopScramble.Length;)
            {
                //Status.Info(string.Join(" ", workshopScramble.Select(w => decisions.TryGetValue(w, out var s) ? s.ToString() : "")));
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
                                    .OrderBy(s =>
                                        decisions
                                            .Where(w => w.Value == s)
                                            .Sum(w => inputData.Workshops[w.Key].max))
                                    .ToList();


                                backtracking.Push(feasibleSlots);
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
        
        private bool SatisfiesSchedulingConstraints(int workshop, int slot, Dictionary<int, int> decisions, InputData inputData)
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

        private bool SatisfiesAssignmentConstraints(int participant, int workshop, int slot, int[] scheduling, Dictionary<int, int[]> fullDecisions, InputData inputData)
        {
            // There can be a situation where a participant is assigned to a dependent workshop (e.g. W1, which is
            // dependent on W2), but in the slot of W2 there is already a workshop of which the participant is a
            // conductor of. To prevent this from happening (because then we have to backtrack really far), we track
            // all slots in which the participant has a fixed workshop and do not allow collisions.
            //
            int[] fixedWorkshop = new int[inputData.SlotCount];
            Array.Fill(fixedWorkshop, -1);
            
            foreach (var constraint in inputData.GetAssignmentConstraintsForParticipant(participant))
            {
                switch (constraint)
                {
                    case SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        if (scheduling[c.Left.Id] != slot && scheduling[c.Right.Id] != slot)
                        {
                            // This decision is not affected by this constraint.
                            //
                            break;
                        }
                        
                        int thisSlot = scheduling[c.Left.Id] == slot ? c.Left.Id : c.Right.Id;
                        int otherSlot = c.Left.Id == thisSlot ? c.Right.Id : c.Left.Id;

                        if (c.Left.Id != workshop && c.Right.Id != workshop &&
                            !fullDecisions[participant].Contains(otherSlot))
                        {
                            // This decision is not affected by this constraint.
                            //
                            break;
                        }
                        
                        if (scheduling[otherSlot] <= slot)
                        {
                            bool committed = fullDecisions[participant].Contains(otherSlot);

                            if (committed && workshop != thisSlot || !committed && workshop == thisSlot)
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (fixedWorkshop[scheduling[otherSlot]] != otherSlot && fixedWorkshop[scheduling[otherSlot]] != -1)
                            {
                                return false;
                            }
                            fixedWorkshop[scheduling[otherSlot]] = otherSlot;
                        }

                        break;
                    }
                    case ContainsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        if (scheduling[c.Element.Id] == slot && c.Element.Id != workshop)
                        {
                            return false;
                        }

                        if (fixedWorkshop[scheduling[c.Element.Id]] != c.Element.Id && fixedWorkshop[scheduling[c.Element.Id]] != -1)
                        {
                            return false;
                        }
                        
                        fixedWorkshop[scheduling[c.Element.Id]] = c.Element.Id;
                        break;
                    }
                    case ContainsNotConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        if (c.Element.Id == workshop)
                        {
                            return false;
                        }
                        break;
                    }
                    case SequenceEqualsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        HashSet<int> distinctWorkshops = new HashSet<int>(
                            fullDecisions[c.Left.Id].Concat(fullDecisions[c.Right.Id]));
                        distinctWorkshops.Add(workshop);

                        if (distinctWorkshops.Count > inputData.SlotCount)
                        {
                            return false;
                        }
                        break;
                    }
                }
            }
            
            return true;
        }

        private bool SatisfiesCriticalSets(InputData inputData, Dictionary<int, int> decisions,
            IEnumerable<CriticalSet> criticalSets)
        {
            foreach (CriticalSet set in criticalSets)
            {
                var coveredSlots = new HashSet<int>();
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