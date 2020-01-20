namespace WSolve.ExtraConditions.Constraints
{
    public class SlotOffsetConstraint : Constraint
    {
        public WorkshopStateless First { get; }
        public WorkshopStateless Second { get; }
        public int Offset { get; }

        public SlotOffsetConstraint(WorkshopStateless first, WorkshopStateless second, int offset)
        {
            First = first;
            Second = second;
            Offset = offset;
        }

        protected override Constraint Negation =>
            throw new ConstraintException("Slot offset constraints can not be negated.");
    }
}