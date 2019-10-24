namespace WSolve
{
    public static class GaSolverMutations
    {
        public class ExchangeAssignment : IMutation
        {
            public ExchangeAssignment(InputData inputData)
            {
                InputData = inputData;
            }

            public InputData InputData { get; }

            public void Mutate(Chromosome chromosome)
            {
                var slot = RNG.NextInt(0, InputData.Slots.Count);
                var p = RNG.NextInts(2, 0, InputData.Participants.Count);
                var s = new int[2];

                for (var i = 0; i < 2; i++)
                for (var si = 0; si < InputData.Slots.Count; si++)
                {
                    var w = chromosome.Workshop(p[0], si);
                    var ws = chromosome.Slot(w);
                    if (ws == slot)
                    {
                        s[i] = si;
                        break;
                    }
                }

                var w0 = chromosome.Workshop(p[0], s[0]);
                var w1 = chromosome.Workshop(p[0], s[0]);

                chromosome.Workshop(p[0], s[0]) = w1;
                chromosome.Workshop(p[1], s[1]) = w0;
            }
        }

        public class ExchangeScheduling : IMutation
        {
            public ExchangeScheduling(InputData inputData)
            {
                InputData = inputData;
            }

            public InputData InputData { get; }

            public void Mutate(Chromosome chromosome)
            {
                var i0 = RNG.NextInt(0, InputData.Workshops.Count);
                var i1 = RNG.NextInt(0, InputData.Workshops.Count);

                var w0 = chromosome.Slot(i0);
                var w1 = chromosome.Slot(i1);

                chromosome.Slot(i0) = w1;
                chromosome.Slot(i1) = w0;

                for (var p = 0; p < InputData.Participants.Count; p++)
                for (var s = 0; s < InputData.Slots.Count; s++)
                {
                    var w = chromosome.Workshop(p, s);
                    if (w == i0)
                        chromosome.Workshop(p, s) = i1;
                    else if (w == i1) chromosome.Workshop(p, s) = i0;
                }
            }
        }

        public class ChangeAssignment : IMutation
        {
            public ChangeAssignment(InputData inputData)
            {
                InputData = inputData;
            }

            public InputData InputData { get; }

            public void Mutate(Chromosome chromosome)
            {
                var s = RNG.NextInt(0, InputData.Slots.Count);
                var p = RNG.NextInt(0, InputData.Participants.Count);

                chromosome.Workshop(p, s) = chromosome.GenerateWorkshopGene();
            }
        }

        public class OptimizeLocally : IMutation
        {
            public OptimizeLocally(IFitness fitness)
            {
                Fitness = fitness;
            }

            public IFitness Fitness { get; }

            public void Mutate(Chromosome chromosome)
            {
                LocalOptimization.Apply(chromosome, Fitness, out var unused, true, 1);
            }
        }
    }
}