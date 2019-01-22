using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public class GaSolverCrossover : ICrossover
    {
        public int ParentCount { get; } = 2;
        public int ChildrenCount { get; } = 2;
        
        public Input Input { get; }

        public GaSolverCrossover(Input input)
        {
            Input = input;
        }
        
        public IEnumerable<Chromosome> Crossover(IReadOnlyList<Chromosome> parents)
        {
            Chromosome p0 = parents[0];
            Chromosome p1 = parents[1];

            bool[] differentSchedule = new bool[Input.Slots.Count];
            for (int i = 0; i < Input.Workshops.Count; i++)
            {
                if (p0.Slot(i) != p1.Slot(i))
                {
                    differentSchedule[p0.Slot(i)] = true;
                    differentSchedule[p1.Slot(i)] = true;
                }
            }

            int[] validExchangeSlots = Enumerable.Range(0, Input.Slots.Count).Where(s => !differentSchedule[s]).ToArray();
            if (differentSchedule.Any(x => x == false))
            {
                int sIdx = RNG.NextInt(0, validExchangeSlots.Length);
                int s = validExchangeSlots[sIdx];
                
                Chromosome c0 = new Chromosome(p0);
                Chromosome c1 = new Chromosome(p1);
                for (int p = 0; p < Input.Participants.Count; p++)
                {
                    int w0 = p0.Workshop(p, s);
                    int w1 = p1.Workshop(p, s);

                    c0.Workshop(p, s) = w1;
                    c1.Workshop(p, s) = w0;
                }

                return new[] {c0, c1};
            }
            else
            {
                return new[] {p0, p1};
            }
        }
    }
}