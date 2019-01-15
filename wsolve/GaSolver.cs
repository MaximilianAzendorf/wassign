using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO.Compression;
using System.Linq;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.CompilerServices;
using GeneticSharp;
using GeneticSharp.Domain;
using GeneticSharp.Domain.Chromosomes;
using GeneticSharp.Domain.Crossovers;
using GeneticSharp.Domain.Fitnesses;
using GeneticSharp.Domain.Mutations;
using GeneticSharp.Domain.Populations;
using GeneticSharp.Domain.Randomizations;
using GeneticSharp.Domain.Reinsertions;
using GeneticSharp.Domain.Selections;
using GeneticSharp.Domain.Terminations;

namespace wsolve
{
    public class GaSolver : ISolver
    {
        private static string pad(string x) => new string(' ', 3 - x.Length) + x;

        public static Input Input { get; private set; }

        private static int pIdx(int p, int s) => Input.Workshops.Count + p * Input.Slots.Count + s;
        private static int wIdx(int w) => w;

        private class SolverChromosome : ChromosomeBase
        {
            public static volatile bool ReturnSelf = true;
            public Input Input { get; }

            public SolverChromosome(Input input)
                : base(input.Workshops.Count + input.Participants.Count * input.Slots.Count)
            {
                Input = input;

                for (int i = 0; i < Length; i++)
                {
                    ReplaceGene(i, GenerateGene(i));
                }
            }

            public int WorkshopOf(int participant, int slot) => (int) GetGene(pIdx(participant, slot)).Value;
            public int SlotOf(int workshop) => (int) GetGene(wIdx(workshop)).Value;

            public int MaxPreferenceUsed
            {
                get
                {
                    int maxPref = 0;
                    for (int p = 0; p < Input.Participants.Count; p++)
                    {
                        for (int s = 0; s < Input.Slots.Count; s++)
                        {
                            maxPref = Math.Max(maxPref, Input.Participants[p].preferences[WorkshopOf(p, s)]);
                        }
                    }

                    return maxPref;
                }
            }

            public int PrefCount(int pref)
            {
                int[] prefCount = new int[Input.Participants.Count];
                for (int p = 0; p < Input.Participants.Count; p++)
                {
                    for (int s = 0; s < Input.Slots.Count; s++)
                    {
                        int w = WorkshopOf(p, s);
                        prefCount[Input.Participants[p].preferences[w]]++;
                    }
                }

                return prefCount[pref];
            }

            public SolverChromosome(Input input, int[] scheduling, int[] assignment)
                : base(input.Workshops.Count + input.Participants.Count * input.Slots.Count)
            {
                Input = input;

                for (int i = 0; i < input.Workshops.Count; i++)
                {
                    ReplaceGene(i, new Gene(scheduling[i]));
                }

                for (int i = 0; i < input.Participants.Count * input.Slots.Count; i++)
                {
                    ReplaceGene(i + input.Workshops.Count, new Gene(assignment[i]));
                }
            }

            public SolverChromosome(Input input, SolverChromosome lhs)
                : base(input.Workshops.Count + input.Participants.Count * input.Slots.Count)
            {
                Input = input;
                for (int i = 0; i < Length; i++)
                {
                    ReplaceGene(i, new Gene(lhs.GetGene(i).Value));
                }
            }

            public override Gene GenerateGene(int geneIndex)
            {
                if(ReturnSelf)
                    return new Gene(GetGene(geneIndex).Value);
                return geneIndex < Input.Workshops.Count
                    ? new Gene(RandomizationProvider.Current.GetInt(0, Input.Slots.Count))
                    : new Gene(RandomizationProvider.Current.GetInt(0, Input.Workshops.Count));
            }

            public override IChromosome CreateNew()
            {
                if(ReturnSelf)
                    return new SolverChromosome(Input, this);
                return new SolverChromosome(Input);
            }
        }
        
        private class SolverFitness : IFitness
        {
            public double Scaling { get; }

            public int AllowedPreference { get; set; } = int.MaxValue;

            public double MajorFactor => Input.Participants.Count * Input.Slots.Count *
                                         Math.Pow(Input.MaxPreference + 1, Options.PreferenceExponent) / Scaling;
            
            public SolverFitness()
            {
                Scaling = Math.Pow(Input.MaxPreference, Options.PreferenceExponent);
            }
            
            private bool IsFeasible(IChromosome chromosome)
            {
                int[] partCounts = new int[Input.Workshops.Count];
                bool[,] isInSlot = new bool[Input.Participants.Count, Input.Slots.Count];
                int[] slots = new int[Input.Workshops.Count];

                for (int i = 0; i < Input.Workshops.Count; i++)
                {
                    int s = (int) chromosome.GetGene(wIdx(i)).Value;
                    slots[i] = s;

                    int conductor = Input.Workshops[i].conductor;
                    bool foundConductor = false;
                    for (int sl = 0; sl < Input.Slots.Count; sl++)
                    {
                        if ((int) chromosome.GetGene(pIdx(conductor, sl)).Value == i)
                        {
                            foundConductor = true;
                            break;
                        }
                    }

                    if (!foundConductor) return false;
                }
                
                for (int i = 0; i < Input.Participants.Count * Input.Slots.Count; i++)
                {
                    int p = i / Input.Slots.Count;
                    int s = i % Input.Slots.Count;
                    int ws = (int)chromosome.GetGene(pIdx(p,s)).Value;
                    if (Input.Participants[p].preferences[ws] > AllowedPreference) return false;
                    if (isInSlot[p, slots[ws]]) return false;
                    isInSlot[p, slots[ws]] = true;
                    partCounts[ws]++;
                }

                for (int i = 0; i < Input.Workshops.Count; i++)
                {
                    if (partCounts[i] < Input.Workshops[i].min) return false;
                    if (partCounts[i] > Input.Workshops[i].max) return false;
                }

                return true;
            }

            public int EvaluateMajor(IChromosome chromosome)
            {
                int m = 0;
                for (int i = 0; i < Input.Participants.Count * Input.Slots.Count; i++)
                {
                    int p = i / Input.Slots.Count;
                    int s = i % Input.Slots.Count;
                    int ws = (int) chromosome.GetGene(pIdx(p,s)).Value;
                    m = Math.Max(m, Input.Participants[p].preferences[ws]);
                }

                return m;
            }
            
            public double EvaluateMinor(IChromosome chromosome)
            {
                if (!IsFeasible(chromosome)) return double.PositiveInfinity;
                
                int[] prefArray = Enumerable.Range(0, Input.MaxPreference + 1).ToArray();
                int[] prefCount = new int[Input.MaxPreference + 1];

                for (int i = 0; i < Input.Participants.Count * Input.Slots.Count; i++)
                {
                    int p = i / Input.Slots.Count;
                    int s = i % Input.Slots.Count;
                    int ws = (int) chromosome.GetGene(pIdx(p,s)).Value;
                    prefCount[Input.Participants[p].preferences[ws]]++;
                }

                return prefCount
                           .Zip(prefArray, (count, pref) => (pref, count))
                           .Sum(p => p.Item2 * Math.Pow(p.Item1+1, Options.PreferenceExponent)) / Scaling;
            }

            public (int, double) EvaluatePair(IChromosome chromosome)
            {
                return (EvaluateMajor(chromosome), EvaluateMinor(chromosome));
            }

            public double Evaluate(IChromosome chromosome)
            {
                return -(EvaluateMajor(chromosome) * MajorFactor + EvaluateMinor(chromosome));
            }
        }
        
        private class SolverMutation : IMutation
        {
            public bool IsOrdered { get; } = false;
            
            public void Mutate(IChromosome chromosome, float probability)
            {
                if (RandomizationProvider.Current.GetFloat() > probability) return;

                switch (RandomizationProvider.Current.GetInt(0, 4))
                {
                    case 0: // Exchange assignment
                    {
                        int slot = RandomizationProvider.Current.GetInt(0, Input.Slots.Count);
                        int[] p = RandomizationProvider.Current.GetUniqueInts(2, 0, Input.Participants.Count);
                        int[] s = new int[2];

                        for (int i = 0; i < 2; i++)
                        {
                            for (int si = 0; si < Input.Slots.Count; si++)
                            {
                                int w = (int)chromosome.GetGene(pIdx(p[0],si)).Value;
                                int ws = (int) chromosome.GetGene(wIdx(w)).Value;
                                if (ws == slot)
                                {
                                    s[i] = si;
                                    break;
                                }
                            }
                        }

                        int i0 = pIdx(p[0],s[0]);
                        int i1 = pIdx(p[1],s[1]);

                        Gene w0 = chromosome.GetGene(i0);
                        Gene w1 = chromosome.GetGene(i1);

                        chromosome.ReplaceGene(i0, w1);
                        chromosome.ReplaceGene(i1, w0);
                        break;
                    }
                    case 1: // Exchange scheduling
                    {
                        int i0 = RandomizationProvider.Current.GetInt(0, Input.Workshops.Count);
                        int i1 = RandomizationProvider.Current.GetInt(0, Input.Workshops.Count);

                        Gene w0 = chromosome.GetGene(i0);
                        Gene w1 = chromosome.GetGene(i1);
                        
                        chromosome.ReplaceGene(i0, w1);
                        chromosome.ReplaceGene(i1, w0);

                        for (int p = 0; p < Input.Participants.Count; p++)
                        {
                            for (int s = 0; s < Input.Slots.Count; s++)
                            {
                                int i = pIdx(p,s);
                                Gene w = chromosome.GetGene(i);
                                if (w.Value.Equals(i0))
                                {
                                    chromosome.ReplaceGene(i, new Gene(i1));
                                }
                                else if (w.Value.Equals(i1))
                                {
                                    chromosome.ReplaceGene(i, new Gene(i0));
                                }
                            }
                        }

                        break;
                    }
                    case 2:
                    case 3: // Change assignment
                    {
                        int slot = RandomizationProvider.Current.GetInt(0, Input.Slots.Count);
                        var p = RandomizationProvider.Current.GetInt(0, Input.Participants.Count);

                        int i = pIdx(p,slot);
                        
                        chromosome.ReplaceGene(i, chromosome.GenerateGene(i));
                        
                        break;
                    }
                }
            }
        }

        private SolverChromosome GreedySolution(Input input)
        {
            List<int>[] slots = new List<int>[input.Slots.Count];
            for (int i = 0; i < slots.Length; i++) slots[i] = new List<int>();

            for (int ws = 0; ws < input.Workshops.Count; ws++)
            {
                slots
                    .OrderBy(s => s.Sum(w => input.Workshops[w].max) / (float) input.Participants.Count)
                    .First()
                    .Add(ws);
            }

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
            
            for (int p = 0; p < input.Participants.Count; p++)
            {
                for (int s = 0; s < input.Slots.Count; s++)
                {
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
            }
            
            return new SolverChromosome(input, scheduling, assignment);
        }

        public GaSolver()
        {
        }

        private IEnumerable<(int, int)> BuildScheduling(SolverChromosome chrom)
        {
            for (int i = 0; i < Input.Workshops.Count; i++)
            {
                yield return (i, (int)chrom.GetGene(wIdx(i)).Value);
            }
        }

        private IEnumerable<(int, int)> BuildAssignment(SolverChromosome chrom)
        {
            for (int p = 0; p < Input.Participants.Count; p++)
            {
                for (int s = 0; s < Input.Slots.Count; s++)
                {
                    yield return (p, (int) chrom.GetGene(pIdx(p,s)).Value);
                }
            }
        }

        public Output Solve(Input input)
        {
            if(Input != null) throw new InvalidOperationException();
            Input = input;
            
            var initc = GreedySolution(input);
            var population = new Population(100, 100, initc);
            SolverFitness fitness = new SolverFitness();
            fitness.Evaluate(initc);
            ISelection selection = new TournamentSelection(5, true);
            ICrossover crossover = new TwoPointCrossover();
            IMutation mutation = new SolverMutation();
            var ga = new GeneticSharp.Domain.GeneticAlgorithm(population, fitness, selection, crossover, mutation);
            ga.Termination = new TimeEvolvingTermination(TimeSpan.FromSeconds(1200));
            ga.MutationProbability = 0.3f;
            ga.CrossoverProbability = 0.7f;

            ga.GenerationRan += (sender, args) =>
            {
                Console.CursorTop = Console.CursorLeft = 0;
                SolverChromosome.ReturnSelf = false;
                fitness.AllowedPreference = Math.Min(fitness.AllowedPreference, ((SolverChromosome) ga.Population.BestChromosome).MaxPreferenceUsed);
                Status.Info($"Generation {ga.GenerationsNumber} finished. Size: {ga.Population.CurrentGeneration.Chromosomes.Count} Best: {fitness.EvaluatePair(ga.BestChromosome)}      ");
                Status.Info($"Pref: {string.Join(" ", Enumerable.Range(0, input.Workshops.Count).Select(((SolverChromosome)ga.Population.BestChromosome).PrefCount).Select(s => pad(s.ToString())))}");
                Status.Info($"Slots:\n\t[{string.Join("]\n\t[", Enumerable.Range(0, input.Slots.Count).Select(s => string.Join(" ", Enumerable.Range(0, input.Workshops.Count).Where(w => ((SolverChromosome)ga.Population.BestChromosome).SlotOf(w) == s).Select(x => pad(x.ToString())))))}]");
            };

            double z = fitness.Evaluate(initc);
            Console.Clear();
            ga.Start();
            fitness.Evaluate(ga.Population.BestChromosome);
            
            var output = new Output(
                BuildScheduling((SolverChromosome)ga.Population.BestChromosome), 
                BuildAssignment((SolverChromosome)ga.Population.BestChromosome));
            Input = null;

            return output;
        }
    }
}