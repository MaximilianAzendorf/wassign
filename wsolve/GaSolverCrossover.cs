using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public class GaSolverCrossover : ICrossover
    {
        public int ParentCount { get; } = 2;
        public int ChildrenCount { get; } = 6;
        
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

            if (differentSchedule.Any(x => x == false))
            {
                List<Chromosome> ret = new List<Chromosome>();
                for(int s = 0; s < Input.Slots.Count; s++)
                {
                    if (differentSchedule[s]) continue;
                    
                    Chromosome c0 = new Chromosome(p0);
                    Chromosome c1 = new Chromosome(p1);
                    for (int p = 0; p < Input.Participants.Count; p++)
                    {
                        int w0 = p0.Workshop(p, s);
                        int w1 = p1.Workshop(p, s);

                        c0.Workshop(p, s) = w1;
                        c1.Workshop(p, s) = w0;
                    }

                    ret.Add(c0);
                    ret.Add(c1);
                }

                return ret;
            }
            else
            {
                return new[] {p0, p0, p0, p1, p1, p1};
            }
        }
    }
}