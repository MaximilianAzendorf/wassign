namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public abstract class Constraint
    {
        internal Constraint() { }
        
        protected abstract Constraint Negation { get; }

        public static Constraint operator !(Constraint constraint)
        {
            return constraint.Negation;
        }
    }
}