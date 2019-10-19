using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection.Metadata.Ecma335;
using System.Threading;

namespace WSolve
{
    public class GreedySolver : ISolver
    {
        private List<int>[] SolveAssignment(InputData inputData, int seed, CancellationToken ctoken)
        {
            Random rnd = new Random(seed);

            int[] workshopScramble = Enumerable.Range(0, inputData.Workshops.Count).OrderBy(x => rnd.Next()).ToArray();
            
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
                    int availableMaxPush = workshopScramble.Skip(depth).Sum(w => inputData.Workshops[w].max);

                    var impossibilities = Enumerable.Range(0, inputData.Slots.Count)
                        .Where(s =>
                            decisions
                                .Where(w => w.slot == s)
                                .Sum(w => inputData.Workshops[w.ws].max) + availableMaxPush < inputData.Participants.Count);

                    if (impossibilities.Any())
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
            
            List<int>[] slots = new List<int>[inputData.Slots.Count];
            for (int i = 0; i < slots.Length; i++) slots[i] = new List<int>();

            foreach (var decision in decisions)
            {
                slots[decision.slot].Add(decision.ws);
            }

            return slots;
        }
        
        public IEnumerable<Solution> SolveIndefinitely(InputData inputData, CancellationToken ctoken)
        {
            Random rootRnd = new Random(Options.Seed ?? 2);

            while (!ctoken.IsCancellationRequested)
            {
                List<int>[] slots = SolveAssignment(inputData, rootRnd.Next(), ctoken);

                if (slots == null) yield break;
                
                int[] scheduling = new int[inputData.Workshops.Count];
                for (int i = 0; i < slots.Length; i++)
                {
                    for (int j = 0; j < slots[i].Count; j++)
                    {
                        scheduling[slots[i][j]] = i;
                    }
                }

                int[] assignment = new int[inputData.Participants.Count * inputData.Slots.Count];
                int[] part = new int[inputData.Workshops.Count];
                Array.Fill(assignment, -1);

                for (int i = 0; i < inputData.Workshops.Count; i++)
                {
                    foreach (int conductor in inputData.Workshops[i].conductors)
                    {
                        assignment[conductor * inputData.Slots.Count + scheduling[i]] = i;
                        part[i]++;
                    }
                }

                Random rnd = new Random(rootRnd.Next());

                var indices = Enumerable.Range(0, inputData.Participants.Count)
                    .SelectMany(p => Enumerable.Range(0, inputData.Slots.Count).Select(s => (p, s)))
                    .OrderBy(x => rnd.Next());

                foreach ((int p, int s) in indices)
                {
                    if (ctoken.IsCancellationRequested) yield break;
                    
                    if (assignment[p * inputData.Slots.Count + s] != -1)
                    {
                        continue;
                    }

                    for (int l = 0; l < 2; l++)
                    {
                        for (int w = 0; w < inputData.Workshops.Count; w++)
                        {
                            if (scheduling[w] != s) continue;

                            int limit = l == 0 ? inputData.Workshops[w].min : inputData.Workshops[w].max;

                            if (part[w] < limit)
                            {
                                assignment[p * inputData.Slots.Count + s] = w;
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

                for (int i = 0; i < inputData.Workshops.Count; i++)
                {
                    outputScheduling.Add((i, scheduling[i]));
                }

                for (int i = 0; i < inputData.Participants.Count; i++)
                {
                    for (int s = 0; s < inputData.Slots.Count; s++)
                    {
                        outputAssignment.Add((i, assignment[i * inputData.Slots.Count + s]));
                    }
                }

                yield return new Solution(inputData, outputScheduling, outputAssignment);
            }
        }

        public Solution Solve(InputData inputData) => SolveIndefinitely(inputData, new CancellationToken()).First();
    }
}