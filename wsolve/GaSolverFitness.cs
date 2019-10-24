using System;
using System.Linq;

namespace WSolve
{
    public class GaSolverFitness : IFitness
    {
        public GaSolverFitness(InputData inputData, Func<Chromosome, bool> extraFilter = null)
        {
            InputData = inputData;
            Scaling = (float) Math.Pow(InputData.MaxPreference, Options.PreferenceExponent);
            ExtraFilter = extraFilter;
        }

        public float Scaling { get; }

        public InputData InputData { get; }

        public Func<Chromosome, bool> ExtraFilter { get; }

        public bool IsFeasible(Chromosome chromosome)
        {
            var partCounts = new int[InputData.Workshops.Count];
            var isInSlot = new bool[InputData.Participants.Count, InputData.Slots.Count];
            var slots = new int[InputData.Workshops.Count];

            for (var i = 0; i < InputData.Workshops.Count; i++)
            {
                slots[i] = chromosome.Slot(i);

                foreach (var conductor in InputData.Workshops[i].conductors)
                {
                    var foundConductor = false;
                    for (var sl = 0; sl < InputData.Slots.Count; sl++)
                        if (chromosome.Workshop(conductor, sl) == i)
                        {
                            foundConductor = true;
                            break;
                        }

                    if (!foundConductor) return false;
                }
            }

            for (var i = 0; i < InputData.Participants.Count * InputData.Slots.Count; i++)
            {
                var p = i / InputData.Slots.Count;
                var ws = chromosome.Workshop(p, i % InputData.Slots.Count);
                if (isInSlot[p, slots[ws]]) return false;

                isInSlot[p, slots[ws]] = true;
                partCounts[ws]++;
            }

            for (var i = 0; i < InputData.Workshops.Count; i++)
            {
                if (partCounts[i] < InputData.Workshops[i].min) return false;

                if (partCounts[i] > InputData.Workshops[i].max) return false;
            }

            if (!(ExtraFilter?.Invoke(chromosome) ?? true)) return false;

            return true;
        }

        public (float major, float minor) Evaluate(Chromosome chromosome)
        {
            if (chromosome == Chromosome.Null) return (float.PositiveInfinity, float.PositiveInfinity);

            float major = EvaluateMajor(chromosome);
            var minor = EvaluateMinor(chromosome);

            if (!float.IsFinite(major) || !float.IsFinite(minor))
                return (float.PositiveInfinity, float.PositiveInfinity);

            return (major, minor);
        }

        public int EvaluateMajor(Chromosome chromosome)
        {
            var m = 0;
            for (var i = 0; i < InputData.Participants.Count * InputData.Slots.Count; i++)
            {
                var p = i / InputData.Slots.Count;
                var ws = chromosome.Workshop(p, i % InputData.Slots.Count);
                m = Math.Max(m, InputData.Participants[p].preferences[ws]);
            }

            return m;
        }

        public float EvaluateMinor(Chromosome chromosome)
        {
            if (!IsFeasible(chromosome)) return float.PositiveInfinity;

            var prefArray = Enumerable.Range(0, InputData.MaxPreference + 1).ToArray();
            var prefCount = new int[InputData.MaxPreference + 1];

            for (var i = 0; i < InputData.Participants.Count * InputData.Slots.Count; i++)
            {
                var p = i / InputData.Slots.Count;
                var ws = chromosome.Workshop(p, i % InputData.Slots.Count);
                prefCount[InputData.Participants[p].preferences[ws]]++;
            }

            return prefCount
                       .Zip(prefArray, (count, pref) => (pref, count))
                       .Sum(p => p.count * (float) Math.Pow(p.pref + 1, Options.PreferenceExponent)) / Scaling;
        }
    }
}