namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public abstract class Constraint
    {
        internal Constraint() { }

        public static Constraint operator !(Constraint constraint) 
            => new NotConstraint(constraint);
    }
}