namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public class SetValueConstraint<TOwner, TFieldType> : Constraint
    {
        public TOwner Owner { get; }
        public TFieldType Value { get; }

        public SetValueConstraint(TOwner owner, TFieldType value)
        {
            Owner = owner;
            Value = value;
        }

        protected override Constraint Negation => new ForbidValueConstraint<TOwner, TFieldType>(Owner, Value);
    }
}