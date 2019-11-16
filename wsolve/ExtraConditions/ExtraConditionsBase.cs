using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using WSolve.ExtraConditions.StatelessAccess.Constraints;

namespace WSolve.ExtraConditions
{
    public abstract class ExtraConditionsBase
    {
        private readonly Chromosome _chromosome;
        private readonly List<Constraint> _staticConstraints = new List<Constraint>();

        protected ExtraConditionsBase(Chromosome chromosome)
        {
            _chromosome = chromosome;
        }

        public bool DirectResult { get; private set; } = true;
        public IEnumerable<Constraint> StaticConstraints => _staticConstraints.AsEnumerable();

        public IReadOnlyCollection<ParticipantAccessor> Participants => Enumerable
            .Range(0, _chromosome.InputData.Participants.Count)
            .Select(n => new ParticipantAccessor(n, this, _chromosome))
            .ToImmutableList();

        public IReadOnlyCollection<WorkshopAccessor> Workshops => Enumerable
            .Range(0, _chromosome.InputData.Workshops.Count)
            .Select(n => new WorkshopAccessor(n, this, _chromosome))
            .ToImmutableList();

        public IReadOnlyCollection<SlotAccessor> Slots => Enumerable
            .Range(0, _chromosome.InputData.Slots.Count)
            .Select(n => new SlotAccessor(n, this, _chromosome))
            .ToImmutableList();

        protected void AddCondition(bool condition)
        {
            DirectResult &= condition;
        }

        protected void AddCondition(Constraint constraint)
        {
            _staticConstraints.Add(constraint);
        }

        public ParticipantAccessor Participant(string nameFragment)
        {
            int res = _chromosome.InputData.Participants.FindIndex(p => p.name == nameFragment);
            if (res >= 0)
            {
                return new ParticipantAccessor(res, this, _chromosome);
            }

            res = _chromosome.InputData.Participants.FindIndex(p => p.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new ParticipantAccessor(res, this, _chromosome);
            }

            res = _chromosome.InputData.Participants.FindIndex(p => p.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new ParticipantAccessor(res, this, _chromosome);
            }

            throw new ArgumentException($"Could not find participant with name fragment '{nameFragment}'.");
        }

        public WorkshopAccessor Workshop(string nameFragment)
        {
            int res = _chromosome.InputData.Workshops.FindIndex(w => w.name == nameFragment);
            if (res >= 0)
            {
                return new WorkshopAccessor(res, this, _chromosome);
            }

            res = _chromosome.InputData.Workshops.FindIndex(w => w.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new WorkshopAccessor(res, this, _chromosome);
            }

            res = _chromosome.InputData.Workshops.FindIndex(w => w.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new WorkshopAccessor(res, this, _chromosome);
            }

            throw new ArgumentException($"Could not find workshop with name fragment '{nameFragment}'.");
        }

        public SlotAccessor Slot(string nameFragment)
        {
            int res = _chromosome.InputData.Slots.FindIndex(s => s == nameFragment);
            if (res >= 0)
            {
                return new SlotAccessor(res, this, _chromosome);
            }

            res = _chromosome.InputData.Slots.FindIndex(s => s.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new SlotAccessor(res, this, _chromosome);
            }

            res = _chromosome.InputData.Slots.FindIndex(s => s.Contains(nameFragment));
            if (res >= 0)
            {
                return new SlotAccessor(res, this, _chromosome);
            }

            throw new ArgumentException($"Could not find slot with name fragment '{nameFragment}'.");
        }
    }
}