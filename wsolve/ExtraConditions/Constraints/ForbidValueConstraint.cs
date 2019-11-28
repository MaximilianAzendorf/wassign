using System.Collections.Generic;

namespace WSolve.ExtraConditions.Constraints
{
    public class ForbidValueConstraint<TOwner, TFieldType> : Constraint
    {
        protected bool Equals(ForbidValueConstraint<TOwner, TFieldType> other)
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

            return Equals((ForbidValueConstraint<TOwner, TFieldType>) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((EqualityComparer<TOwner>.Default.GetHashCode(Owner) * 397) ^ EqualityComparer<TFieldType>.Default.GetHashCode(Value)) * 271;
            }
        }

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