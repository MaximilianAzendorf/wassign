using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace wsolve
{
    public class GaSolver : ISolver
    {
        public Output Solve(Input input)
        {
            GeneticAlgorithm ga = new GeneticAlgorithm
            {
                Fitness = new GaSolverFitness(input),
                Crossover = new GaSolverCrossover(input),
                Selection = new EliteSelection(),
                Reinsertion = new EliteReinsertion(),
                SelectionSize = new ConstantParameter<int>(200),
                PopulationSize = new ConstantParameter<int>(2000),
                Terminate = new ConstantParameter<bool>(false),
                MutationChance = new ConstantParameter<float>(0.15f),
                CrossoverChance = new ConstantParameter<float>(0.8f),
            };
            
            ga.Mutations.Add(1, new GaSolverMutations.ChangeAssignment(input));
            ga.Mutations.Add(1, new GaSolverMutations.ExchangeAssignment(input));
            ga.Mutations.Add(2, new GaSolverMutations.ExchangeScheduling(input));

            Status.Info("Generating initial generation.");
            var init = new GreedySolver().SolveIndefinitely(input, CancellationToken.None).Take(1000)
                .Select(output => Chromosome.FromOutput(input, output)).ToArray();
            
            Status.Info("Starting GA.");
            Task.Run(() =>
            {
                while (true)
                {
                    Status.Info($"GEN={ga.Generations.Count} FIT={ga.BestFitness}");
                    Task.Delay(100).Wait();
                }
            });
            ga.Run(init);

            return null;
        }
    }
}