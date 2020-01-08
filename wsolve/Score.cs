using System;
using System.Collections.Generic;
using System.Linq;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.Constraints;

namespace WSolve
{
    public class Score : IScore
    {
        public Score(InputData inputData)
        {
            InputData = inputData;
            Scaling = (float) Math.Pow(InputData.MaxPreference, Options.PreferenceExponent);
        }

        public float Scaling { get; }

        public InputData InputData { get; }

        private bool SatisfiesConstraints(Candidate candidate)
        {
            foreach (var constraint in candidate.InputData.SchedulingConstraints)
            {
                switch (constraint)
                {
                    case SetValueConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        if(candidate.Slot(c.Owner.Id) != c.Value.Id) return false;
                        break;
                    }
                    case ForbidValueConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        if(candidate.Slot(c.Owner.Id) == c.Value.Id) return false;
                        break;
                    }
                    case EqualsConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        if(candidate.Slot(c.Left.Id) != candidate.Slot(c.Right.Id)) return false;
                        break;
                    }
                    case EqualsNotConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        if(candidate.Slot(c.Left.Id) == candidate.Slot(c.Right.Id)) return false;
                        break;
                    }
                    case SlotOffsetConstraint c:
                    {
                        if (candidate.Slot(c.Second.Id) - candidate.Slot(c.First.Id) != c.Offset) return false;
                        break;
                    }
                    default:
                    {
                        throw new ArgumentException($"Unknown constraint type {constraint}.");
                    }
                }
            }

            foreach (var constraint in candidate.InputData.AssignmentConstraints)
            {
                switch (constraint)
                {
                    case SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        if (!candidate.Participants(c.Left.Id).SequenceEqual(candidate.Participants(c.Right.Id)))
                        {
                            return false;
                        }
                        break;
                    }
                    case ContainsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        if (!candidate.Workshops(c.Owner.Id).Contains(c.Element.Id))
                        {
                            return false;
                        }
                        break;
                    }
                    case ContainsNotConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        if (candidate.Workshops(c.Owner.Id).Contains(c.Element.Id))
                        {
                            return false;
                        }
                        break;
                    }
                    case SequenceEqualsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        if (!candidate.Workshops(c.Left.Id).SequenceEqual(candidate.Workshops(c.Right.Id)))
                        {
                            return false;
                        }
                        break;
                    }
                }
            }
            
            return true;
        }
        
        public bool IsFeasible(Candidate candidate)
        {
            var partCounts = new int[InputData.Workshops.Count];
            var isInSlot = new bool[InputData.Participants.Count, InputData.Slots.Count];
            var slots = new int[InputData.Workshops.Count];

            if (!SatisfiesConstraints(candidate))
            {
                return false;
            }

            for (int i = 0; i < InputData.WorkshopCount; i++)
            {
                slots[i] = candidate.Slot(i);
            }

            for (int i = 0; i < InputData.Participants.Count * InputData.Slots.Count; i++)
            {
                int p = i / InputData.Slots.Count;
                int ws = candidate.Workshop(p, i % InputData.Slots.Count);
                if (isInSlot[p, slots[ws]])
                {
                    return false;
                }

                isInSlot[p, slots[ws]] = true;
                partCounts[ws]++;
            }

            for (int i = 0; i < InputData.Workshops.Count; i++)
            {
                if (partCounts[i] < InputData.Workshops[i].min)
                {
                    return false;
                }

                if (partCounts[i] > InputData.Workshops[i].max)
                {
                    return false;
                }
            }

            return true;
        }

        public (float major, float minor) Evaluate(Candidate candidate)
        {
            if (candidate == Candidate.Null)
            {
                return (float.PositiveInfinity, float.PositiveInfinity);
            }

            float major = EvaluateMajor(candidate);
            float minor = EvaluateMinor(candidate);

            if (!float.IsFinite(major) || !float.IsFinite(minor))
            {
                return (float.PositiveInfinity, float.PositiveInfinity);
            }

            return (major, minor);
        }

        public int EvaluateMajor(Candidate candidate)
        {
            int m = 0;
            for (int i = 0; i < InputData.Participants.Count * InputData.Slots.Count; i++)
            {
                int p = i / InputData.Slots.Count;
                int ws = candidate.Workshop(p, i % InputData.Slots.Count);
                m = Math.Max(m, InputData.Participants[p].preferences[ws]);
            }

            return m;
        }

        public float EvaluateMinor(Candidate candidate)
        {
            if (!IsFeasible(candidate))
            {
                return float.PositiveInfinity;
            }

            int[] prefArray = Enumerable.Range(0, InputData.MaxPreference + 1).ToArray();
            var prefCount = new int[InputData.MaxPreference + 1];

            for (int i = 0; i < InputData.Participants.Count * InputData.Slots.Count; i++)
            {
                int p = i / InputData.Slots.Count;
                int ws = candidate.Workshop(p, i % InputData.Slots.Count);
                prefCount[InputData.Participants[p].preferences[ws]]++;
            }

            return prefCount
                       .Zip(prefArray, (count, pref) => (pref, count))
                       .Sum(p => p.count * (float) Math.Pow(p.pref + 1, Options.PreferenceExponent)) / Scaling;
        }
    }
}