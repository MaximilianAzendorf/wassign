namespace WSolve
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Threading;

    public class GaSolver : ISolver
    {
        public Solution Solve(InputData inputData)
        {
            IFitness fitness = new GaSolverFitness(inputData, GetExtraConditions(inputData));

            CriticalSetAnalysis criticalSetAnalysis;

            if (!Options.NoCriticalSets)
            {
                Status.Info("Performing critical set analysis.");
                criticalSetAnalysis = new CriticalSetAnalysis(inputData);
                Status.Info($"{criticalSetAnalysis.AllSets.Count()} critical sets found, Preference limit is {criticalSetAnalysis.PreferenceBound}.");
            }
            else
            {
                Status.Info("Skipping critical set analysis.");
                criticalSetAnalysis = CriticalSetAnalysis.Empty(inputData);
            }

            Chromosome res;
            if (Options.NoGeneticOptimizations)
            {
                int tries = 0;
                using (var solutionSource = new GreedySolver().SolveIndefinitely(inputData, criticalSetAnalysis, CancellationToken.None)
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
                    ApplyStaticPrefPumpHeuristic(res);
                }
                else if (res.MaxUsedPreference == criticalSetAnalysis.PreferenceBound)
                {
                    Status.Info("Reached preference bound; preference pump not needed.");
                }
                else
                {
                    Status.Info("Skipping preference pump heuristic.");
                }
            }
            else
            {
                MultiLevelGaSystem ga = new MultiLevelGaSystem(inputData, criticalSetAnalysis, Options.BucketSize)
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

                ga.Mutations.Add(30, new GaSolverMutations.ChangeAssignment(inputData));
                ga.Mutations.Add(8, new GaSolverMutations.ExchangeAssignment(inputData));
                ga.Mutations.Add(4, new GaSolverMutations.ExchangeScheduling(inputData));
                if (!Options.NoLocalOptimizations)
                {
                    ga.Mutations.Add(1, new GaSolverMutations.OptimizeLocally(fitness));
                }

                ga.Start();

                res = ga.WaitForSolutionChromosome(TimeSpan.FromMilliseconds(1000));
            }

            if (!Options.NoLocalOptimizations)
            {
                bool retry;
                do
                {
                    retry = false;
                    res = LocalOptimization.Apply(res, fitness, out int altCount);

                    Status.Info($"Local Optimizations made {altCount} alteration(s).");

                    if (!Options.NoPrefPump && altCount > 0 && res.MaxUsedPreference != criticalSetAnalysis.PreferenceBound)
                    {
                        Status.Info("Retrying preference pumping because local optimizations made changes.");
                        retry = true;

                        ApplyStaticPrefPumpHeuristic(res);
                    }
                } while (retry);
            }
            else
            {
                Status.Info("Skipping local optimizations.");
            }

            Status.Info("Final Fitness: " + fitness.Evaluate(res));
            return res.ToSolution();
        }

        private Func<Chromosome, bool> GetExtraConditions(InputData inputData)
        {
            if (Options.ExtraConditions == null)
            {
                return null;
            }

            string condition = File.Exists(Options.ExtraConditions)
                ? File.ReadAllText(Options.ExtraConditions)
                : $"AddCondition({Options.ExtraConditions});";

            Status.Info("Compiling extra conditions.");
            return ExtraConditionsCompiler.Compile(condition, inputData);
        }
        
        private void ApplyStaticPrefPumpHeuristic(Chromosome chromosome)
        {
            int maxPref = chromosome.MaxUsedPreference;
            
            foreach (int pref in chromosome.InputData.Participants.SelectMany(p => p.preferences).Distinct()
                .OrderBy(x => -x))
            {
                if (pref > maxPref)
                {
                    continue;
                }

                if (PrefPumpHeuristic.TryPump(
                        chromosome, 
                        pref, 
                        Options.PreferencePumpMaxDepth,
                        TimeSpan.FromSeconds(Options.PreferencePumpTimeoutSeconds)) != PrefPumpResult.Success)
                {
                    Status.Info($"Preference pump heuristic could not pump preference {pref}.");
                    break;
                }
            }
        }
    }
}