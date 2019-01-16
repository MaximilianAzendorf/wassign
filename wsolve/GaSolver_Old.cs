using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO.Compression;
using System.Linq;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.CompilerServices;

namespace wsolve
{/*
    public class GaSolver_Old : ISolver
    {
        private static string pad(string x) => new string(' ', 3 - x.Length) + x;

        public static Input Input { get; private set; }

        private static int pIdx(int p, int s) => Input.Workshops.Count + p * Input.Slots.Count + s;
        private static int wIdx(int w) => w;

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
    }*/
}