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
                int slot = RNG.NextInt(0, InputData.Slots.Count);
                int[] p = RNG.NextInts(2, 0, InputData.Participants.Count);
                var s = new int[2];

                for (int i = 0; i < 2; i++)
                {
                    for (int si = 0; si < InputData.Slots.Count; si++)
                    {
                        int w = chromosome.Workshop(p[0], si);
                        int ws = chromosome.Slot(w);
                        if (ws == slot)
                        {
                            s[i] = si;
                            break;
                        }
                    }
                }

                int w0 = chromosome.Workshop(p[0], s[0]);
                int w1 = chromosome.Workshop(p[0], s[0]);

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
                int i0 = RNG.NextInt(0, InputData.Workshops.Count);
                int i1 = RNG.NextInt(0, InputData.Workshops.Count);

                int w0 = chromosome.Slot(i0);
                int w1 = chromosome.Slot(i1);

                chromosome.Slot(i0) = w1;
                chromosome.Slot(i1) = w0;

                for (int p = 0; p < InputData.Participants.Count; p++)
                {
                    for (int s = 0; s < InputData.Slots.Count; s++)
                    {
                        int w = chromosome.Workshop(p, s);
                        if (w == i0)
                        {
                            chromosome.Workshop(p, s) = i1;
                        }
                        else if (w == i1)
                        {
                            chromosome.Workshop(p, s) = i0;
                        }
                    }
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
                int s = RNG.NextInt(0, InputData.Slots.Count);
                int p = RNG.NextInt(0, InputData.Participants.Count);

                chromosome.Workshop(p, s) = RNG.NextInt(0, chromosome.InputData.Workshops.Count);
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
                LocalOptimization.Apply(chromosome, Fitness, out int unused, true, 1);
            }
        }
    }
}