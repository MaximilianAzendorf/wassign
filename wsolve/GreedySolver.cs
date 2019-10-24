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

            while (!ctoken.IsCancellationRequested)
            {
                var preferenceLimit = csAnalysis.PreferenceBound;

                List<int>[] slots = null;
                while (slots == null)
                {
                    var sets = csAnalysis.ForPreference(preferenceLimit).ToArray();

                    var timeout = TimeSpan.FromSeconds(Options.CriticalSetTimeoutSeconds);

                    slots = SolveAssignment(inputData, sets, timeout, rootRnd.Next(), ctoken);

                    if (slots == null)
                        try
                        {
                            preferenceLimit = inputData.PreferenceLevels.SkipWhile(p => p <= preferenceLimit).First();
                        }
                        catch (InvalidOperationException)
                        {
                            preferenceLimit = int.MaxValue;
                        }
                }

                var scheduling = new int[inputData.Workshops.Count];
                for (var i = 0; i < slots.Length; i++)
                for (var j = 0; j < slots[i].Count; j++)
                    scheduling[slots[i][j]] = i;

                var assignment = new int[inputData.Participants.Count * inputData.Slots.Count];
                var part = new int[inputData.Workshops.Count];
                Array.Fill(assignment, -1);

                for (var i = 0; i < inputData.Workshops.Count; i++)
                    foreach (var conductor in inputData.Workshops[i].conductors)
                    {
                        assignment[conductor * inputData.Slots.Count + scheduling[i]] = i;
                        part[i]++;
                    }

                var rnd = new Random(rootRnd.Next());

                var indices = Enumerable.Range(0, inputData.Participants.Count)
                    .SelectMany(p => Enumerable.Range(0, inputData.Slots.Count).Select(s => (p, s)))
                    .OrderBy(x => rnd.Next());

                foreach (var (p, s) in indices)
                {
                    if (ctoken.IsCancellationRequested) yield break;

                    if (assignment[p * inputData.Slots.Count + s] != -1) continue;

                    for (var l = 0; l < 2; l++)
                    for (var w = 0; w < inputData.Workshops.Count; w++)
                    {
                        if (scheduling[w] != s) continue;

                        var limit = l == 0 ? inputData.Workshops[w].min : inputData.Workshops[w].max;

                        if (part[w] < limit)
                        {
                            assignment[p * inputData.Slots.Count + s] = w;
                            part[w]++;
                            goto assigned;
                        }
                    }

                    assigned: ;
                }

                var outputScheduling = new List<(int workshop, int slot)>();
                var outputAssignment = new List<(int participant, int workshop)>();

                for (var i = 0; i < inputData.Workshops.Count; i++) outputScheduling.Add((i, scheduling[i]));

                for (var i = 0; i < inputData.Participants.Count; i++)
                for (var s = 0; s < inputData.Slots.Count; s++)
                    outputAssignment.Add((i, assignment[i * inputData.Slots.Count + s]));

                yield return new Solution(inputData, outputScheduling, outputAssignment);
            }
        }

        private List<int>[] SolveAssignment(InputData inputData, CriticalSet[] criticalSets, TimeSpan timeout, int seed,
            CancellationToken ctoken)
        {
            var startTime = DateTime.Now;
            var rnd = new Random(seed);

            var workshopScramble = Enumerable.Range(0, inputData.Workshops.Count).OrderBy(x => rnd.Next()).ToArray();

            Stack<(int ws, int slot)> decisions = new Stack<(int, int)>();

            var backtracking = new Stack<List<int>>();

            for (var depth = 0; depth < workshopScramble.Length;)
            {
                if (ctoken.IsCancellationRequested) return null;

                if (DateTime.Now > startTime + timeout) return null;

                // no solution
                if (depth == -1) return null;

                var workshop = workshopScramble[depth];

                if (backtracking.Count <= depth)
                {
                    var availableMaxPush = workshopScramble.Skip(depth).Sum(w => inputData.Workshops[w].max);

                    var impossibilities = Enumerable.Range(0, inputData.Slots.Count)
                        .Where(s =>
                            decisions
                                .Where(w => w.slot == s)
                                .Sum(w => inputData.Workshops[w.ws].max) + availableMaxPush <
                            inputData.Participants.Count);

                    if (impossibilities.Any())
                    {
                        backtracking.Push(new List<int>());
                    }
                    else
                    {
                        if (!SatisfiesCriticalSets(inputData, decisions, criticalSets))
                        {
                            backtracking.Push(new List<int>());
                        }
                        else
                        {
                            var feasibleSlots = Enumerable.Range(0, inputData.Slots.Count)
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

                var nextSlot = backtracking.Peek().First();
                backtracking.Peek().Remove(nextSlot);
                decisions.Push((workshop, nextSlot));
                depth++;
            }

            var slots = new List<int>[inputData.Slots.Count];
            for (var i = 0; i < slots.Length; i++) slots[i] = new List<int>();

            foreach (var decision in decisions) slots[decision.slot].Add(decision.ws);

            return slots;
        }

        private bool SatisfiesCriticalSets(InputData inputData, Stack<(int ws, int slot)> decisions,
            IEnumerable<CriticalSet> criticalSets)
        {
            var decisionMap = decisions.ToDictionary(d => d.ws, d => d.slot);

            foreach (var set in criticalSets)
            {
                var coveredSlots = new HashSet<int>();
                var missing = 0;

                foreach (var element in set)
                    if (!decisionMap.TryGetValue(element, out var slot))
                        missing++;
                    else
                        coveredSlots.Add(slot);

                if (coveredSlots.Count + missing < inputData.Slots.Count) return false;
            }

            return true;
        }
    }
}