namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public class NotConstraint : Constraint
    {
        public Constraint Child { get; }

        public NotConstraint(Constraint child)
        {
            Child = child;
        }
    }
}