namespace wsolve
{
    public static class GaSolverMutations
    {
        public class ExchangeAssignment : IMutation
        {
            public Input Input { get; }

            public ExchangeAssignment(Input input)
            {
                Input = input;
            }
            
            public void Mutate(Chromosome chromosome)
            {
                int slot = RNG.NextInt(0, Input.Slots.Count);
                int[] p = RNG.NextInts(2, 0, Input.Participants.Count);
                int[] s = new int[2];

                for (int i = 0; i < 2; i++)
                {
                    for (int si = 0; si < Input.Slots.Count; si++)
                    {
                        int w = (int)chromosome.Workshop(p[0],si);
                        int ws = (int) chromosome.Slot(w);
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
            public Input Input { get; }

            public ExchangeScheduling(Input input)
            {
                Input = input;
            }

            public void Mutate(Chromosome chromosome)
            {
                int i0 = RNG.NextInt(0, Input.Workshops.Count);
                int i1 = RNG.NextInt(0, Input.Workshops.Count);

                int w0 = chromosome.Slot(i0);
                int w1 = chromosome.Slot(i1);
                        
                chromosome.Slot(i0) = w1;
                chromosome.Slot(i1) = w0;

                for (int p = 0; p < Input.Participants.Count; p++)
                {
                    for (int s = 0; s < Input.Slots.Count; s++)
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
            public Input Input { get; }

            public ChangeAssignment(Input input)
            {
                Input = input;
            }
            
            public void Mutate(Chromosome chromosome)
            {
                int s = RNG.NextInt(0, Input.Slots.Count);
                var p = RNG.NextInt(0, Input.Participants.Count);

                chromosome.Workshop(p, s) = chromosome.GenerateWorkshopGene();
            }
        }
    }
}