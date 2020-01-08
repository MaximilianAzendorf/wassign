using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions.Constraints
{
    public abstract class Constraint
    {
        public static readonly IImmutableSet<Type> SchedulingConstraintTypes = new HashSet<Type>
        {
            typeof(ContainsConstraint<SlotStateless, WorkshopStateless>), // will be reduced
            typeof(ContainsNotConstraint<SlotStateless, WorkshopStateless>), // will be reduced
            typeof(SequenceEqualsConstraint<SlotStateless, WorkshopStateless>), // will be reduced
            typeof(SetValueConstraint<WorkshopStateless, SlotStateless>),
            typeof(ForbidValueConstraint<WorkshopStateless, SlotStateless>),
            typeof(EqualsConstraint<WorkshopStateless, SlotStateless>),
            typeof(EqualsNotConstraint<WorkshopStateless, SlotStateless>),
            typeof(SlotOffsetConstraint),
        }.ToImmutableHashSet();

        public static readonly IImmutableSet<Type> AssignmentConstraintTypes = new HashSet<Type>
        {
            typeof(ContainsConstraint<WorkshopStateless, ParticipantStateless>), // will be reduced
            typeof(ContainsNotConstraint<WorkshopStateless, ParticipantStateless>), // will be reduced
            typeof(SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless>),
            typeof(ContainsConstraint<ParticipantStateless, WorkshopStateless>),
            typeof(ContainsNotConstraint<ParticipantStateless, WorkshopStateless>),
            typeof(SequenceEqualsConstraint<ParticipantStateless, WorkshopStateless>),
        }.ToImmutableHashSet();
        
        internal Constraint() { }
        
        protected abstract Constraint Negation { get; }

        public static Constraint operator !(Constraint constraint)
        {
            return constraint.Negation;
        }

        public static List<int[]> GetDependentWorkshops(IEnumerable<Constraint> constraints, InputData inputData)
        {
            UnionFind<int> workshopGroups = new UnionFind<int>(Enumerable.Range(0, inputData.WorkshopCount));

            foreach (var constraint in constraints
                .OfType<SequenceEqualsConstraint<WorkshopStateless, ParticipantStateless>>())
            {
                workshopGroups.Union(constraint.Left.Id, constraint.Right.Id);
            }

            return Enumerable.Range(0, inputData.WorkshopCount)
                .GroupBy(workshopGroups.Find)
                .Select(g => g.ToArray())
                .ToList();
        }

        private static List<int[]> GetMandatoryCriticalSets(IEnumerable<Constraint> constraints, InputData inputData)
        {
            List<int[]> workshopGroups = new List<int[]>();

            foreach (var constGroup in constraints
                .OfType<ContainsConstraint<ParticipantStateless, WorkshopStateless>>()
                .GroupBy(c => c.Owner.Id))
            {
                workshopGroups.Add(constGroup.Select(c => c.Element.Id).ToArray());
            }

            return workshopGroups;
        }

        private static IEnumerable<Constraint> ExpandDependentConstraints(IEnumerable<Constraint> constraintsEnumerable,
            InputData inputData)
        {
            Constraint[] constraints = constraintsEnumerable as Constraint[] ?? constraintsEnumerable.ToArray();
            var dependentWorkshops = GetDependentWorkshops(constraints, inputData);
            var mandatoryCritSets = GetMandatoryCriticalSets(constraints, inputData);
            
            foreach (var group in dependentWorkshops.Concat(mandatoryCritSets))
            {
                for (int i = 0; i < group.Length; i++)
                {
                    for (int j = i + 1; j < group.Length; j++)
                    {
                        yield return new WorkshopStateless(group[i], inputData).Slot !=
                                     new WorkshopStateless(group[j], inputData).Slot;
                    }
                }
            }
            
            foreach (var constraint in constraints
                .OfType<ContainsConstraint<ParticipantStateless, WorkshopStateless>>())
            {
                var group = dependentWorkshops.SingleOrDefault(g => g.Contains(constraint.Element.Id));
                
                if(group == null) continue;

                foreach (var w in group)
                {
                    if(w == constraint.Element.Id) continue;

                    yield return constraint.Owner.Events.Contains(new WorkshopStateless(w, inputData));
                }
            }
            
            foreach (var constraint in constraints
                .OfType<ContainsNotConstraint<ParticipantStateless, WorkshopStateless>>())
            {
                var group = dependentWorkshops.SingleOrDefault(g => g.Contains(constraint.Element.Id));
                
                if(group == null) continue;

                foreach (var w in group)
                {
                    if(w == constraint.Element.Id) continue;

                    yield return !constraint.Owner.Events.Contains(new WorkshopStateless(w, inputData));
                }
            }

            foreach (var constraint in constraints)
            {
                yield return constraint;
            }
        }
        
        public static IEnumerable<Constraint> ReduceAndOptimize(IEnumerable<Constraint> constraints, InputData inputData, out bool isInfeasible)
        {
            isInfeasible = false;
            List<Constraint> res = new List<Constraint>();
            
            foreach (var constraint in constraints)
            {
                switch (constraint)
                {
                    case ContainsConstraint<SlotStateless, WorkshopStateless> c:
                    {
                        res.Add(new SetValueConstraint<WorkshopStateless, SlotStateless>(c.Element, c.Owner));
                        break;
                    }
                    case ContainsNotConstraint<SlotStateless, WorkshopStateless> c:
                    {
                        res.Add(new ForbidValueConstraint<WorkshopStateless, SlotStateless>(c.Element, c.Owner));
                        break;
                    }
                    case ContainsConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        res.Add(new ContainsConstraint<ParticipantStateless, WorkshopStateless>(c.Element, c.Owner));
                        break;
                    }
                    case ContainsNotConstraint<WorkshopStateless, ParticipantStateless> c:
                    {
                        res.Add(new ContainsNotConstraint<ParticipantStateless, WorkshopStateless>(c.Element, c.Owner));
                        break;
                    }
                    // The following constraint is always either a tautology or a contradiction.
                    //
                    case SequenceEqualsConstraint<SlotStateless, WorkshopStateless> c: 
                    {
                        if (c.Left.Id != c.Right.Id)
                        {
                            isInfeasible = true;
                        }
                        break;
                    }

                    default:
                    {
                        res.Add(constraint);
                        break;
                    }
                }
            }
            
            return ExpandDependentConstraints(res, inputData).Distinct();
        }
        
        public static IEnumerable<Constraint> EventSeries(InputData inputData, IEnumerable<int> workshops)
        {
            var workshopsArray = workshops.ToArray();
            var accessors = workshopsArray.Select(w => new WorkshopStateless(w, inputData)).ToArray();
            for (int i = 0; i < workshopsArray.Length; i++)
            {
                for (int j = i + 1; j < workshopsArray.Length; j++)
                {
                    yield return accessors[i].Participants == accessors[j].Participants;
                    yield return new SlotOffsetConstraint(accessors[i], accessors[j], j - i);
                }
            }
        }
    }
}