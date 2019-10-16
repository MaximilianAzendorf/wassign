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
            MultiLevelGaSystem ga = new MultiLevelGaSystem(inputData, Options.BucketSize)
            {
                Fitness = new GaSolverFitness(inputData, GetExtraConditions()),
                Crossover =  new GaSolverCrossover(inputData),
                Selection = Options.Selection,
                Timeout = TimeSpan.FromSeconds(Options.TimeoutSeconds),
                PopulationSize = Parameter.Create(g => (int)Options.PopulationSize.GetValue(g.Progress / Options.FinalPhaseStart)),
                MutationChance = Parameter.Create(g => (float)Options.MutationChance.GetValue(g.Progress / Options.FinalPhaseStart)),
                CrossoverChance = Parameter.Create(g => (float)Options.CrossoverChance.GetValue(g.Progress / Options.FinalPhaseStart)),
            };
            
            ga.Mutations.Add(15, new GaSolverMutations.ChangeAssignment(inputData));
            ga.Mutations.Add(3, new GaSolverMutations.ExchangeAssignment(inputData));
            ga.Mutations.Add(1, new GaSolverMutations.ExchangeScheduling(inputData));
            
            ga.Start();
            
            var res = ga.WaitForSolution(TimeSpan.FromSeconds(0.5));
            return res;
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