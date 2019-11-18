using System;
using System.IO;
using System.Linq;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.StatelessAccess;

namespace WSolve
{
    public abstract class SolverBase : ISolver
    {
        protected CriticalSetAnalysis GetCsAnalysis(InputData inputData)
        {
            CriticalSetAnalysis criticalSetAnalysis;
            
            if (!Options.NoCriticalSets)
            {
                Status.Info("Performing critical set analysis.");
                criticalSetAnalysis = new CriticalSetAnalysis(inputData);
                Status.Info(
                    $"{criticalSetAnalysis.AllSets.Count()} critical sets found, Preference limit is {criticalSetAnalysis.PreferenceBound}.");
            }
            else
            {
                Status.Info("Skipping critical set analysis.");
                criticalSetAnalysis = CriticalSetAnalysis.Empty(inputData);
            }

            return criticalSetAnalysis;
        }
        
        private Func<Chromosome, ExtraConditionsBase> GetExtraConditions(InputData inputData, bool stateless)
        {
            if (Options.ExtraConditions == null)
            {
                return null;
            }

            string condition = File.Exists(Options.ExtraConditions)
                ? File.ReadAllText(Options.ExtraConditions)
                : $"AddCondition({Options.ExtraConditions});";

            Status.Info("Compiling extra conditions.");
            return ExtraConditionsCompiler.Compile(condition, inputData, stateless);
        }

        protected Func<Chromosome, bool> GetExtraConditions(InputData inputData)
        {
            var extraConditions = GetExtraConditions(inputData, false);
            
            if (extraConditions == null)
            {
                return null;
            }
            else
            {
                return c => extraConditions(c).DirectResult;
            }
        }

        protected Func<Chromosome, CustomExtraConditionsBaseStateless> GetExtraConditionsStateless(InputData inputData)
        {
            var extraConditions = GetExtraConditions(inputData, true);

            if (extraConditions == null)
            {
                return null;
            }

            return chromosome => (CustomExtraConditionsBaseStateless)extraConditions(chromosome);
        }
        
        public abstract Solution Solve(InputData inputData);
    }
}