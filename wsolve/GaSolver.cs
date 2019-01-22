using System;
using System.Linq;
using System.Numerics;
using System.Threading;
using System.Threading.Tasks;

namespace wsolve
{
    public class GaSolver : ISolver
    {
        private class TimeParam : IParameter<int>
        {
            public TimeSpan Span { get; set; }
            public int Start { get; set; }
            public int End { get; set; }
            
            public double Exp { get; set; }

            public readonly DateTime StartTime = DateTime.Now;
            
            public int Evalutate(GaLevel algorithmState)
            {
                double f = (DateTime.Now - StartTime).TotalSeconds / Span.TotalSeconds;

                f = Math.Min(1, Math.Max(0, Math.Pow(f, Exp)));

                return (int)(Start + (End - Start) * f);
            }
        }

        private float lerp(float min, float max, float f, float exp = 1f) => min + (max - min) * Math.Max(0, Math.Min(1, (float)Math.Pow(f, exp)));
    
        public Output Solve(Input input)
        {
            //var init = new GreedySolver().SolveIndefinitely(input, CancellationToken.None).Take(50000)
            //    .Select(output => Chromosome.FromOutput(input, output)).ToArray();
            /*
            var init = PrefPumpPresolver.Presolve(input, TimeSpan.FromMinutes(0.5));
            
            GaLevel ga = new GaLevel
            {
                Fitness = new GaSolverFitness(input),
                Crossover = new GaSolverCrossover(input),
                Selection = new EliteSelection(),
                Reinsertion = new EliteReinsertion(),
                SelectionSize = new TimeParam {Span = TimeSpan.FromMinutes(1), Start = 500, End = 5, Exp = 0.23},
                PopulationSize = new TimeParam {Span = TimeSpan.FromMinutes(1), Start = 1000, End = 10, Exp = 0.23},
                Termination = Parameter.Create(g => g.StagnationFactor >= 0.5f),
                MutationChance = Parameter.Create(0.15f),
                CrossoverChance = Parameter.Create(0.3f),
                PrefPumpChance =  Parameter.Create(g => lerp(0.0f, 1f, g.StagnationFactor)),
                PrefPumpDepth =  Parameter.Create(g => (int)lerp(5, 15, g.StagnationFactor)),
                PrefPumpTimeout =  Parameter.Create(g => TimeSpan.FromMilliseconds((int)lerp(50, 1500, g.StagnationFactor, 1.8f))),
            };
            
            ga.Mutations.Add(15, new GaSolverMutations.ChangeAssignment(input));
            ga.Mutations.Add(3, new GaSolverMutations.ExchangeAssignment(input));
            ga.Mutations.Add(1, new GaSolverMutations.ExchangeScheduling(input));

            
            Status.Info("Starting GA.");
            ga.Run(init);
            Status.Info("GA Finished.");

            Chromosome best = ga.BestChromosome;
            
            return new Output(
                Enumerable.Range(0, input.Workshops.Count).Select(w => (w, best.Slot(w))),
                Enumerable.Range(0, input.Participants.Count).SelectMany(p =>
                    Enumerable.Range(0, input.Slots.Count).Select(s => (p, best.Workshop(p,s)))));
                    */

            MultiLevelGaSystem ga = new MultiLevelGaSystem(input, 5000)
            {
                Fitness = new GaSolverFitness(input),
                Crossover =  new GaSolverCrossover(input),
                Selection = new EliteSelection(),
                PopulationSize = Parameter.Create(720),
                Termination =  Parameter.Create(g => g.System.StagnationFactor > 0.5f),
                MutationChance = Parameter.Create(0.15f),
                CrossoverChance = Parameter.Create(0.3f),
                PrefPumpChance = Parameter.Create(g => lerp(0.0f, 1f, g.StagnationFactor)),
                PrefPumpDepth = Parameter.Create(g => (int)lerp(5, 15, g.StagnationFactor)),
                PrefPumpTimeout =  Parameter.Create(g => TimeSpan.FromMilliseconds((int)lerp(50, 1500, g.StagnationFactor, 1.8f))),
            };
            
            ga.Mutations.Add(15, new GaSolverMutations.ChangeAssignment(input));
            ga.Mutations.Add(3, new GaSolverMutations.ExchangeAssignment(input));
            ga.Mutations.Add(1, new GaSolverMutations.ExchangeScheduling(input));
            
            ga.Start();
            
            return ga.WaitForSolution(TimeSpan.FromSeconds(0.5));
        }
    }
}