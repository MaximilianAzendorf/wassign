using System;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;

namespace WSolve
{
    public class GaSolver : ISolver
    {
        public Solution Solve(InputData inputData)
        {
            IFitness fitness = new GaSolverFitness(inputData, GetExtraConditions());
            
            Chromosome res;
            if (Options.NoGeneticOptimizations)
            {
                int tries = 0;
                using (var solutionSource = new GreedySolver().SolveIndefinitely(inputData, CancellationToken.None)
                    .GetEnumerator())
                {
                    Status.Info("Skipping genetic optimization; just computing greedy solution.");
                    do
                    {
                        if (tries == 1)
                        {
                            Status.Warning("Greedy solution was not feasible on first try.");
                        }

                        solutionSource.MoveNext();
                        res = Chromosome.FromOutput(inputData, solutionSource.Current);
                        tries++;
                    } while (!fitness.IsFeasible(res));
                }

                if (tries > 1)
                {
                    Status.Info($"Needed {tries} tries to find feasible greedy solution.");
                }

                if (!Options.NoPrefPump)
                {
                    Status.Info("Applying preference pump heuristic.");
                    foreach (int pref in inputData.Participants.SelectMany(p => p.preferences).Distinct()
                        .OrderBy(x => -x))
                    {
                        Status.Info($"Trying to pump preference {pref}.");
                        if (PrefPumpHeuristic.TryPump(res, pref, Options.PreferencePumpMaxDepth,
                                TimeSpan.FromSeconds(Options.PreferencePumpTimeoutSeconds)) != PrefPumpResult.Success)
                        {
                            Status.Info($"Preference pump heuristic could not pump preference {pref}.");
                            break;
                        }
                    }
                }
                else
                {
                    Status.Info("Skipping preference pump heuristic.");
                }
            }
            else
            {
                MultiLevelGaSystem ga = new MultiLevelGaSystem(inputData, Options.BucketSize)
                {
                    Fitness = fitness,
                    Crossover = new GaSolverCrossover(inputData),
                    Selection = Options.Selection,
                    Timeout = TimeSpan.FromSeconds(Options.TimeoutSeconds),
                    PopulationSize = Parameter.Create(g =>
                        (int) Options.PopulationSize.GetValue(g.Progress / Options.FinalPhaseStart)),
                    MutationChance = Parameter.Create(g =>
                        (float) Options.MutationChance.GetValue(g.Progress / Options.FinalPhaseStart)),
                    CrossoverChance = Parameter.Create(g =>
                        (float) Options.CrossoverChance.GetValue(g.Progress / Options.FinalPhaseStart)),
                };

                ga.Mutations.Add(15, new GaSolverMutations.ChangeAssignment(inputData));
                ga.Mutations.Add(3, new GaSolverMutations.ExchangeAssignment(inputData));
                ga.Mutations.Add(1, new GaSolverMutations.ExchangeScheduling(inputData));

                ga.Start();

                res = ga.WaitForSolutionChromosome(TimeSpan.FromMilliseconds(1000));
            }

            if (!Options.NoLocalOptimizations)
            {
                res = LocalOptimization.Apply(res, fitness, out int altCount);

                Status.Info($"Local Optimizations made {altCount} alteration(s).");
            }
            else
            {
                Status.Info("Skipping local optimizations.");
            }

            Status.Info("Final Fitness: " + fitness.Evaluate(res));
            return res.ToSolution();
        }

        private Func<Chromosome, bool> GetExtraConditions()
        {
            if(Options.ExtraConditions == null) return null;

            string condition = File.Exists(Options.ExtraConditions)
                ? File.ReadAllText(Options.ExtraConditions)
                : $"AddCondition({Options.ExtraConditions});";

            Status.Info("Compiling extra conditions.");
            return ExtraConditionsCompiler.Compile(condition);
        }
    }
}