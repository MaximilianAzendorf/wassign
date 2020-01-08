using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.Constraints;

namespace WSolve
{
    public class InputData
    {
        public static readonly string GeneratedPrefix = "~";
        public static readonly string NotScheduledSlotPrefix = GeneratedPrefix + "not_scheduled_";
        public static readonly string HiddenWorkshopPrefix = GeneratedPrefix + "hidden_";
        
        public InputData(MutableInputData data, bool buildConstraints = true)
        {
            Workshops = data.Workshops.ToImmutableList();
            Participants = data.Participants.Select(p => (p.name, (IReadOnlyList<int>)p.preferences.ToImmutableList())).ToImmutableList();
            Slots = data.Slots.ToImmutableList();

            if (!Slots.Any())
            {
                Slots = new[] {"Generated slot"}.ToImmutableList();
            }

            var constraints = new List<Constraint>();
            if (buildConstraints)
            {
                constraints.AddRange(data.CompileConstraints());
                
                constraints.AddRange(GetConductorConstraints(data));

                constraints.AddRange(GetPartConstraints());

                constraints = Constraint.ReduceAndOptimize(constraints, this, out bool isInfeasible).ToList();

                if (isInfeasible)
                {
                    throw new ArgumentException("The given constraints are not satisfiable.");
                }

                var newLimits = GetDependentWorkshopLimits(constraints);
                var newPrefs = GetDependentPreferences(constraints);

                Workshops = Enumerable.Range(0, WorkshopCount)
                    .Select(w => (Workshops[w].name, newLimits[w].min, newLimits[w].max, Workshops[w].continuation))
                    .ToImmutableList();

                Participants = Enumerable.Range(0, ParticipantCount)
                    .Select(p => (Participants[p].name, (IReadOnlyList<int>)newPrefs[p].ToImmutableList()))
                    .ToImmutableList();
            }

            SchedulingConstraints = constraints
                .Where(c => Constraint.SchedulingConstraintTypes.Contains(c.GetType()))
                .ToImmutableList();
            AssignmentConstraints = constraints
                .Where(c => Constraint.AssignmentConstraintTypes.Contains(c.GetType()))
                .ToImmutableList();
            
            workshopConstraintMap = GetSchedulingConstraintMap();
            participantConstraintMap = GetAssignmentConstraintMap();

            DependentWorkshopGroups = Constraint.GetDependentWorkshops(constraints, this)
                .Select(g => (IReadOnlyCollection<int>)g.ToImmutableList())
                .ToImmutableList();

            PreferenceLevels = Participants.SelectMany(p => p.preferences).Distinct().OrderBy(x => x).ToImmutableList();
        }

        public IEnumerable<Constraint> GetSchedulingConstraintsForWorkshop(int workshopId)
        {
            return workshopConstraintMap[workshopId];
        }

        public IEnumerable<Constraint> GetAssignmentConstraintsForParticipant(int participantId)
        {
            return participantConstraintMap[participantId];
        }

        public IReadOnlyList<(string name, int min, int max, int? continuation)> Workshops { get; }

        public IReadOnlyList<(string name, IReadOnlyList<int> preferences)> Participants { get; }

        public IReadOnlyList<string> Slots { get; }
        
        public IReadOnlyList<Constraint> SchedulingConstraints { get; }
        public IReadOnlyList<Constraint> AssignmentConstraints { get; }
        
        public IReadOnlyList<IReadOnlyCollection<int>> DependentWorkshopGroups { get; }
        
        public int MaxPreference => Participants.Any() ? Participants.Max(p => p.preferences.Max()) : 0;

        public IReadOnlyList<int> PreferenceLevels;

        public int WorkshopCount => Workshops.Count;
        public int ParticipantCount => Participants.Count;
        public int SlotCount => Slots.Count;

        private readonly IReadOnlyDictionary<int, IReadOnlyList<Constraint>> workshopConstraintMap;
        private readonly IReadOnlyDictionary<int, IReadOnlyList<Constraint>> participantConstraintMap;

        private IReadOnlyDictionary<int, IReadOnlyList<Constraint>> GetSchedulingConstraintMap()
        {
            // Create a dictionary to quickly get all constraints that affect a given workshop.
            //
            Dictionary<int, List<Constraint>> map =
                Enumerable.Range(0, WorkshopCount).ToDictionary(w => w, _ => new List<Constraint>());

            foreach (var constraint in SchedulingConstraints)
            {
                switch (constraint)
                {
                    case SetValueConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        map[c.Owner.Id].Add(constraint);
                        break;
                    }
                    case ForbidValueConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        map[c.Owner.Id].Add(constraint);
                        break;
                    }
                    case EqualsConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        map[c.Left.Id].Add(constraint);
                        map[c.Right.Id].Add(constraint);
                        break;
                    }
                    case EqualsNotConstraint<WorkshopStateless, SlotStateless> c:
                    {
                        map[c.Left.Id].Add(constraint);
                        map[c.Right.Id].Add(constraint);
                        break;
                    }
                    case SlotOffsetConstraint c:
                    {
                        map[c.First.Id].Add(constraint);
                        map[c.Second.Id].Add(constraint);
                        break;
                    }
                }
            }

            return map.ToDictionary(k => k.Key, k => (IReadOnlyList<Constraint>) k.Value.ToImmutableList())
                .ToImmutableDictionary();
        }

        private IReadOnlyDictionary<int, IReadOnlyList<Constraint>> GetAssignmentConstraintMap()
        {
            Dictionary<int, List<Constraint>> map =
                Enumerable.Range(0, ParticipantCount).ToDictionary(w => w, _ => new List<Constraint>());

            foreach (var constraint in AssignmentConstraints)
            {
                switch (constraint)
                {
                    case SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        foreach (var list in map.Values)
                        {
                            list.Add(constraint);
                        }
                        break;
                    }
                    case ContainsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        map[c.Owner.Id].Add(constraint);
                        break;
                    }
                    case ContainsNotConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        map[c.Owner.Id].Add(constraint);
                        break;
                    }
                    case SequenceEqualsConstraint<ParticipantStateless, WorkshopStateless> c:
                    {
                        map[c.Left.Id].Add(constraint);
                        map[c.Right.Id].Add(constraint);
                        break;
                    }
                }
            }

            return map.ToDictionary(k => k.Key, k => (IReadOnlyList<Constraint>) k.Value.ToImmutableList())
                .ToImmutableDictionary();
        }

        private IEnumerable<Constraint> GetConductorConstraints(MutableInputData inputData)
        {
            Dictionary<int, List<int>> conductorMap = new Dictionary<int, List<int>>();
            
            foreach ((int participant, int workshop) in inputData.Conductors)
            {
                yield return new WorkshopStateless(workshop, this).Participants.Contains(
                        new ParticipantStateless(participant, this));

                if (!conductorMap.TryGetValue(participant, out var list))
                {
                    conductorMap.Add(participant, list = new List<int>());
                }

                list.Add(workshop);
            }

            foreach (var list in conductorMap.Values.Where(l => l.Count > 1))
            {
                for (int i = 0; i < list.Count; i++)
                {
                    for (int j = i + 1; j < list.Count; j++)
                    {
                        yield return new WorkshopStateless(list[i], this).Slot != new WorkshopStateless(list[j], this).Slot;
                    }
                }
            }
        }

        private IEnumerable<Constraint> GetPartConstraints()
        {
            UnionFind<int> workshopGroups = new UnionFind<int>(Enumerable.Range(0, WorkshopCount));

            for (int w = 0; w < WorkshopCount; w++)
            {
                if (Workshops[w].continuation != null)
                {
                    workshopGroups.Union(w, Workshops[w].continuation.Value);
                }
            }

            foreach (var group in Enumerable.Range(0, WorkshopCount).GroupBy(w => workshopGroups.Find(w)))
            {
                foreach (var c in Constraint.EventSeries(this, group))
                {
                    yield return c;
                }
            }
        }

        private (int min, int max)[] GetDependentWorkshopLimits(IEnumerable<Constraint> constraints)
        {
            UnionFind<int> workshopGroups = new UnionFind<int>(Enumerable.Range(0, WorkshopCount));

            foreach (var constraint in constraints
                .OfType<SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless>>())
            {
                workshopGroups.Union(constraint.Left.Id, constraint.Right.Id);
            }
            
            (int min, int max)[] limits = new (int min, int max)[WorkshopCount];

            Array.Fill(limits, (0, int.MaxValue));
            
            for (int w = 0; w < WorkshopCount; w++)
            {
                int idx = workshopGroups.Find(w);
                limits[idx] = (Math.Max(limits[idx].min, Workshops[w].min),
                    Math.Min(limits[idx].max, Workshops[w].max));
            }

            for (int w = 0; w < WorkshopCount; w++)
            {
                limits[w] = limits[workshopGroups.Find(w)];
            }
            
            return limits;
        }

        private int[][] GetDependentPreferences(IEnumerable<Constraint> constraints)
        {
            var depGroups = Constraint.GetDependentWorkshops(constraints, this);

            int[][] pref = Participants.Select(p => p.preferences.ToArray()).ToArray();

            for (int p = 0; p < ParticipantCount; p++)
            {
                foreach (var group in depGroups)
                {
                    int min = int.MaxValue;

                    foreach (var w in group)
                    {
                        min = Math.Min(min, pref[p][w]);
                    }

                    foreach (var w in group)
                    {
                        pref[p][w] = min;
                    }
                }
            }
            
            return pref;
        }
    }
}