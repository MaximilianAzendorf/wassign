using System;
using System.Linq;

namespace wsolve
{
    public class GaSolverFitness : IFitness
    {
        public float Scaling { get; }
        
        public Input Input { get; }

        public GaSolverFitness(Input input)
        {
            Input = input;
            Scaling = (float)Math.Pow(Input.MaxPreference, Options.PreferenceExponent);
        }

        public bool IsFeasible(Chromosome chromosome) => IsFeasible(chromosome, false);
        public bool IsFeasible(Chromosome chromosome, bool fuckme)
        {
            int[] partCounts = new int[Input.Workshops.Count];
            bool[,] isInSlot = new bool[Input.Participants.Count, Input.Slots.Count];
            int[] slots = new int[Input.Workshops.Count];

            for (int i = 0; i < Input.Workshops.Count; i++)
            {
                slots[i] = chromosome.Slot(i);

                int conductor = Input.Workshops[i].conductor;
                bool foundConductor = false;
                for (int sl = 0; sl < Input.Slots.Count; sl++)
                {
                    if (chromosome.Workshop(conductor, sl) == i)
                    {
                        foundConductor = true;
                        break;
                    }
                }

                if (!foundConductor)
                {
                    if(fuckme) Status.Error("Conductor not found.");
                    return false;
                }
            }
                
            for (int i = 0; i < Input.Participants.Count * Input.Slots.Count; i++)
            {
                int p = i / Input.Slots.Count;
                int ws = chromosome.Workshop(p, i % Input.Slots.Count);
                if (isInSlot[p, slots[ws]])
                {
                    if(fuckme) Status.Error("Multiple workshops in single slot.");
                    return false;
                }
                isInSlot[p, slots[ws]] = true;
                partCounts[ws]++;
            }

            for (int i = 0; i < Input.Workshops.Count; i++)
            {
                if (partCounts[i] < Input.Workshops[i].min)
                {
                    if(fuckme) Status.Error("Too few humans.");
                    return false;
                }

                if (partCounts[i] > Input.Workshops[i].max)
                {
                    if(fuckme) Status.Error("Too many humans.");
                    return false;
                }
            }

            return true;
        }
        
        public int EvaluateMajor(Chromosome chromosome)
        {
            int m = 0;
            for (int i = 0; i < Input.Participants.Count * Input.Slots.Count; i++)
            {
                int p = i / Input.Slots.Count;
                int ws = chromosome.Workshop(p, i % Input.Slots.Count);
                m = Math.Max(m, Input.Participants[p].preferences[ws]);
            }

            return m;
        }
        
        public float EvaluateMinor(Chromosome chromosome)
        {
            if (!IsFeasible(chromosome)) return float.PositiveInfinity;
                
            int[] prefArray = Enumerable.Range(0, Input.MaxPreference + 1).ToArray();
            int[] prefCount = new int[Input.MaxPreference + 1];

            for (int i = 0; i < Input.Participants.Count * Input.Slots.Count; i++)
            {
                int p = i / Input.Slots.Count;
                int ws = chromosome.Workshop(p, i % Input.Slots.Count);
                prefCount[Input.Participants[p].preferences[ws]]++;
            }

            return prefCount
                       .Zip(prefArray, (count, pref) => (pref, count))
                       .Sum(p => p.Item2 * (float)Math.Pow(p.Item1 + 1, Options.PreferenceExponent)) / Scaling;
        }

        
        public (float major, float minor) Evaluate(Chromosome chromosome)
        {
            if (chromosome == Chromosome.Null) return (float.PositiveInfinity, float.PositiveInfinity);
        
            float major = EvaluateMajor(chromosome);
            float minor = EvaluateMinor(chromosome);

            if (!float.IsFinite(major) || !float.IsFinite(minor))
                return (float.PositiveInfinity, float.PositiveInfinity);
            return (major, minor);
        }
    }
}