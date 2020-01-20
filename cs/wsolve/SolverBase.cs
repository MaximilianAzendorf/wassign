using System;
using System.IO;
using System.Linq;
using WSolve.ExtraConditions;

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
        
        public abstract Solution Solve(InputData inputData);
    }
}