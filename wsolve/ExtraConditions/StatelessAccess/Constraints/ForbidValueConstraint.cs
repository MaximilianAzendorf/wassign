namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public class ForbidValueConstraint<TOwner, TFieldType> : Constraint
    {
        public TOwner Owner { get; }
        public TFieldType Value { get; }

        public ForbidValueConstraint(TOwner owner, TFieldType value)
        {
            Owner = owner;
            Value = value;
        }

        protected override Constraint Negation => new SetValueConstraint<TOwner, TFieldType>(Owner, Value);
    }
}