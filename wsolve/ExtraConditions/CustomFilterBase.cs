using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using WSolve.ExtraConditions.Constraints;

namespace WSolve.ExtraConditions
{
    public abstract class CustomFilterBase
    {
        private readonly List<Constraint> _staticConstraints = new List<Constraint>();
        private readonly InputData _inputData;
        
        private Chromosome _chromosome;

        protected CustomFilterBase(InputData inputData)
        {
            _inputData = inputData;
        }

        public bool GetFilterResult(Chromosome chromosome)
        {
            _chromosome = chromosome;
            return Evaluate();
        }

        protected abstract bool Evaluate();

        public IReadOnlyCollection<Participant> Participants => Enumerable
            .Range(0, _inputData.Participants.Count)
            .Select(n => new Participant(n, _chromosome))
            .ToImmutableList();

        public IReadOnlyCollection<Workshop> Workshops => Enumerable
            .Range(0, _inputData.Workshops.Count)
            .Select(n => new Workshop(n, _chromosome))
            .ToImmutableList();

        public IReadOnlyCollection<Slot> Slots => Enumerable
            .Range(0, _inputData.Slots.Count)
            .Select(n => new Slot(n, _chromosome))
            .ToImmutableList();

        public Participant Participant(string nameFragment)
        {
            int res = _inputData.Participants.FindIndex(p => p.name == nameFragment);
            if (res >= 0)
            {
                return new Participant(res, _chromosome);
            }

            res = _inputData.Participants.FindIndex(p => p.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new Participant(res, _chromosome);
            }

            res = _inputData.Participants.FindIndex(p => p.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new Participant(res, _chromosome);
            }

            throw new ArgumentException($"Could not find participant with name fragment '{nameFragment}'.");
        }

        public Workshop Workshop(string nameFragment)
        {
            int res = _inputData.Workshops.FindIndex(w => w.name == nameFragment);
            if (res >= 0)
            {
                return new Workshop(res, _chromosome);
            }

            res = _inputData.Workshops.FindIndex(w => w.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new Workshop(res, _chromosome);
            }

            res = _inputData.Workshops.FindIndex(w => w.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new Workshop(res, _chromosome);
            }

            throw new ArgumentException($"Could not find workshop with name fragment '{nameFragment}'.");
        }

        public Slot Slot(string nameFragment)
        {
            int res = _inputData.Slots.FindIndex(s => s == nameFragment);
            if (res >= 0)
            {
                return new Slot(res, _chromosome);
            }

            res = _inputData.Slots.FindIndex(s => s.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new Slot(res, _chromosome);
            }

            res = _inputData.Slots.FindIndex(s => s.Contains(nameFragment));
            if (res >= 0)
            {
                return new Slot(res, _chromosome);
            }

            throw new ArgumentException($"Could not find slot with name fragment '{nameFragment}'.");
        }
    }
}