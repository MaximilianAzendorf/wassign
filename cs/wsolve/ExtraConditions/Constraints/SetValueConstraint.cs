using System.Collections.Generic;

namespace WSolve.ExtraConditions.Constraints
{
    public class SetValueConstraint<TOwner, TFieldType> : Constraint
    {
        protected bool Equals(SetValueConstraint<TOwner, TFieldType> other)
        {
            return EqualityComparer<TOwner>.Default.Equals(Owner, other.Owner) && EqualityComparer<TFieldType>.Default.Equals(Value, other.Value);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return Equals((SetValueConstraint<TOwner, TFieldType>) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((EqualityComparer<TOwner>.Default.GetHashCode(Owner) * 397) ^ EqualityComparer<TFieldType>.Default.GetHashCode(Value)) * 691;
            }
        }

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