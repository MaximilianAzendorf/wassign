using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection.Metadata.Ecma335;
using System.Threading;

namespace wsolve
{
    public class GreedySolver : ISolver
    {
        private List<int>[] SolveAssignment(Input input, int seed, CancellationToken ctoken)
        {
            Random rnd = new Random(seed);

            int[] workshopScramble = Enumerable.Range(0, input.Workshops.Count).OrderBy(x => rnd.Next()).ToArray();
            
            Stack<(int ws, int slot)> decisions = new Stack<(int, int)>();

            Stack<List<int>> backtracking = new Stack<List<int>>();
            
            for(int depth = 0; depth < workshopScramble.Length;)
            {
                if (ctoken.IsCancellationRequested)
                    return null;
                
                if (depth == -1) // no solution
                    return null;
                
                int workshop = workshopScramble[depth];
                
                if(backtracking.Count <= depth)
                {
                    int availableMaxPush = workshopScramble.Skip(depth).Sum(w => input.Workshops[w].max);

                    var impossibilities = Enumerable.Range(0, input.Slots.Count)
                        .Where(s =>
                            decisions
                                .Where(w => w.slot == s)
                                .Sum(w => input.Workshops[w.ws].max) + availableMaxPush < input.Participants.Count);

                    if (impossibilities.Any())
                    {
                        backtracking.Push(new List<int>());
                    }
                    else
                    {
                        var feasibleSlots = Enumerable.Range(0, input.Slots.Count)
                            .Where(s =>
                                decisions
                                    .Where(w => w.slot == s)
                                    .Sum(w => input.Workshops[w.ws].min) + input.Workshops[workshop].min <=
                                input.Participants.Count)
                            .OrderBy(s =>
                                decisions
                                    .Where(w => w.slot == s)
                                    .Sum(w => input.Workshops[w.ws].max))
                            .ToArray();

                        backtracking.Push(feasibleSlots.ToList());
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
            
            List<int>[] slots = new List<int>[input.Slots.Count];
            for (int i = 0; i < slots.Length; i++) slots[i] = new List<int>();

            foreach (var decision in decisions)
            {
                slots[decision.slot].Add(decision.ws);
            }

            return slots;
        }
        
        public IEnumerable<Output> SolveIndefinitely(Input input, CancellationToken ctoken)
        {
            Random rootRnd = new Random(Options.Seed ?? 0);

            while (!ctoken.IsCancellationRequested)
            {
                List<int>[] slots = SolveAssignment(input, rootRnd.Next(), ctoken);

                if (slots == null) yield break;
                
                int[] scheduling = new int[input.Workshops.Count];
                for (int i = 0; i < slots.Length; i++)
                {
                    for (int j = 0; j < slots[i].Count; j++)
                    {
                        scheduling[slots[i][j]] = i;
                    }
                }

                int[] assignment = new int[input.Participants.Count * input.Slots.Count];
                int[] part = new int[input.Workshops.Count];
                Array.Fill(assignment, -1);

                for (int i = 0; i < input.Workshops.Count; i++)
                {
                    assignment[input.Workshops[i].conductor * input.Slots.Count + scheduling[i]] = i;
                    part[i]++;
                }

                Random rnd = new Random(rootRnd.Next());

                var indices = Enumerable.Range(0, input.Participants.Count)
                    .SelectMany(p => Enumerable.Range(0, input.Slots.Count).Select(s => (p, s)))
                    .OrderBy(x => rnd.Next());

                foreach ((int p, int s) in indices)
                {
                    if (ctoken.IsCancellationRequested) yield break;
                    
                    if (assignment[p * input.Slots.Count + s] != -1)
                    {
                        continue;
                    }

                    for (int l = 0; l < 2; l++)
                    {
                        for (int w = 0; w < input.Workshops.Count; w++)
                        {
                            if (scheduling[w] != s) continue;

                            int limit = l == 0 ? input.Workshops[w].min : input.Workshops[w].max;

                            if (part[w] < limit)
                            {
                                assignment[p * input.Slots.Count + s] = w;
                                part[w]++;
                                goto assigned;
                            }
                        }
                    }

                    assigned:
                    continue;
                }

                List<(int workshop, int slot)> outputScheduling = new List<(int workshop, int slot)>();
                List<(int participant, int workshop)> outputAssignment = new List<(int participant, int workshop)>();

                for (int i = 0; i < input.Workshops.Count; i++)
                {
                    outputScheduling.Add((i, scheduling[i]));
                }

                for (int i = 0; i < input.Participants.Count; i++)
                {
                    for (int s = 0; s < input.Slots.Count; s++)
                    {
                        outputAssignment.Add((i, assignment[i * input.Slots.Count + s]));
                    }
                }

                yield return new Output(outputScheduling, outputAssignment);
            }
        }

        public Output Solve(Input input) => SolveIndefinitely(input, new CancellationToken()).First();
    }
}