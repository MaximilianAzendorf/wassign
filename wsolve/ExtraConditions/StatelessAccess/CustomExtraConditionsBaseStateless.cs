using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using WSolve.ExtraConditions.StatelessAccess.Constraints;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public abstract class CustomExtraConditionsBaseStateless : ExtraConditionsBase
    {
        private readonly List<Constraint> _staticConstraints = new List<Constraint>();

        protected CustomExtraConditionsBaseStateless(Chromosome chromosome)
            : base(chromosome)
        {
        }

        public IEnumerable<Constraint> StaticConstraints => _staticConstraints.AsEnumerable();

        public IReadOnlyCollection<ParticipantAccessorStateless> Participants => Enumerable
            .Range(0, _chromosome.InputData.Participants.Count)
            .Select(n => new ParticipantAccessorStateless(n, this, _chromosome))
            .ToImmutableList();

        public IReadOnlyCollection<WorkshopAccessorStateless> Workshops => Enumerable
            .Range(0, _chromosome.InputData.Workshops.Count)
            .Select(n => new WorkshopAccessorStateless(n, this, _chromosome))
            .ToImmutableList();

        public IReadOnlyCollection<SlotAccessorStateless> Slots => Enumerable
            .Range(0, _chromosome.InputData.Slots.Count)
            .Select(n => new SlotAccessorStateless(n, this, _chromosome))
            .ToImmutableList();

        protected void AddCondition(Constraint constraint)
        {
            _staticConstraints.Add(constraint);
        }

        public ParticipantAccessorStateless Participant(string nameFragment)
        {
            int res = _chromosome.InputData.Participants.FindIndex(p => p.name == nameFragment);
            if (res >= 0)
            {
                return new ParticipantAccessorStateless(res, this, _chromosome);
            }

            res = _chromosome.InputData.Participants.FindIndex(p => p.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new ParticipantAccessorStateless(res, this, _chromosome);
            }

            res = _chromosome.InputData.Participants.FindIndex(p => p.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new ParticipantAccessorStateless(res, this, _chromosome);
            }

            throw new ArgumentException($"Could not find participant with name fragment '{nameFragment}'.");
        }

        public WorkshopAccessorStateless Workshop(string nameFragment)
        {
            int res = _chromosome.InputData.Workshops.FindIndex(w => w.name == nameFragment);
            if (res >= 0)
            {
                return new WorkshopAccessorStateless(res, this, _chromosome);
            }

            res = _chromosome.InputData.Workshops.FindIndex(w => w.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new WorkshopAccessorStateless(res, this, _chromosome);
            }

            res = _chromosome.InputData.Workshops.FindIndex(w => w.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new WorkshopAccessorStateless(res, this, _chromosome);
            }

            throw new ArgumentException($"Could not find workshop with name fragment '{nameFragment}'.");
        }

        public SlotAccessorStateless Slot(string nameFragment)
        {
            int res = _chromosome.InputData.Slots.FindIndex(s => s == nameFragment);
            if (res >= 0)
            {
                return new SlotAccessorStateless(res, this, _chromosome);
            }

            res = _chromosome.InputData.Slots.FindIndex(s => s.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new SlotAccessorStateless(res, this, _chromosome);
            }

            res = _chromosome.InputData.Slots.FindIndex(s => s.Contains(nameFragment));
            if (res >= 0)
            {
                return new SlotAccessorStateless(res, this, _chromosome);
            }

            throw new ArgumentException($"Could not find slot with name fragment '{nameFragment}'.");
        }
    }
}