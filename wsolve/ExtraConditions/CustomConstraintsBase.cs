using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using WSolve.ExtraConditions.Constraints;

namespace WSolve.ExtraConditions
{
    public abstract class CustomConstraintsBase
    {
        private readonly List<Constraint> _staticConstraints = new List<Constraint>();
        private readonly InputData _inputData;

        protected CustomConstraintsBase(InputData inputData)
        {
            _inputData = inputData;
        }

        public IEnumerable<Constraint> GetStaticConstraints()
        {
            _staticConstraints.Clear();
            Evaluate();
            return _staticConstraints.AsEnumerable();
        }

        protected abstract void Evaluate();

        public IReadOnlyCollection<ParticipantStateless> Participants => Enumerable
            .Range(0, _inputData.Participants.Count)
            .Select(n => new ParticipantStateless(n, _inputData))
            .ToImmutableList();

        public IReadOnlyCollection<WorkshopStateless> Workshops => Enumerable
            .Range(0, _inputData.Workshops.Count)
            .Select(n => new WorkshopStateless(n, _inputData))
            .ToImmutableList();

        public IReadOnlyCollection<SlotStateless> Slots => Enumerable
            .Range(0, _inputData.Slots.Count)
            .Select(n => new SlotStateless(n, _inputData))
            .ToImmutableList();

        protected void AddConstraint(Constraint constraint)
        {
            _staticConstraints.Add(constraint);
        }

        protected void AddConstraint(IEnumerable<Constraint> constraints)
        {
            _staticConstraints.AddRange(constraints);
        }

        protected void AddConstraint(params Constraint[] constraints)
        {
            _staticConstraints.AddRange(constraints);
        }

        public ParticipantStateless Participant(string nameFragment)
        {
            int res = _inputData.Participants.FindIndex(p => p.name == nameFragment);
            if (res >= 0)
            {
                return new ParticipantStateless(res, _inputData);
            }

            res = _inputData.Participants.FindIndex(p => p.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new ParticipantStateless(res, _inputData);
            }

            res = _inputData.Participants.FindIndex(p => p.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new ParticipantStateless(res, _inputData);
            }

            throw new ArgumentException($"Could not find participant with name fragment '{nameFragment}'.");
        }

        public WorkshopStateless Workshop(string nameFragment)
        {
            int res = _inputData.Workshops.FindIndex(w => w.name == nameFragment);
            if (res >= 0)
            {
                return new WorkshopStateless(res, _inputData);
            }

            res = _inputData.Workshops.FindIndex(w => w.name.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new WorkshopStateless(res, _inputData);
            }

            res = _inputData.Workshops.FindIndex(w => w.name.Contains(nameFragment));
            if (res >= 0)
            {
                return new WorkshopStateless(res, _inputData);
            }

            throw new ArgumentException($"Could not find workshop with name fragment '{nameFragment}'.");
        }

        public SlotStateless Slot(string nameFragment)
        {
            int res = _inputData.Slots.FindIndex(s => s == nameFragment);
            if (res >= 0)
            {
                return new SlotStateless(res, _inputData);
            }

            res = _inputData.Slots.FindIndex(s => s.StartsWith(nameFragment));
            if (res >= 0)
            {
                return new SlotStateless(res, _inputData);
            }

            res = _inputData.Slots.FindIndex(s => s.Contains(nameFragment));
            if (res >= 0)
            {
                return new SlotStateless(res, _inputData);
            }

            throw new ArgumentException($"Could not find slot with name fragment '{nameFragment}'.");
        }

        protected IEnumerable<Constraint> EventSeries(params WorkshopStateless[] workshops)
        {
            for (int i = 0; i < workshops.Length; i++)
            {
                for (int j = i + 1; j < workshops.Length; j++)
                {
                    yield return workshops[i].Participants == workshops[j].Participants;
                    yield return new SlotOffsetConstraint(workshops[i], workshops[j], j - i);
                }
            }
        }
    }
}