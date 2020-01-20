using System.Collections.Generic;

namespace WSolve.ExtraConditions.Constraints
{
    public class EqualsConstraint<TOwner, TFieldType> : Constraint
    {
        protected bool Equals(EqualsConstraint<TOwner, TFieldType> other)
        {
            return (EqualityComparer<TOwner>.Default.Equals(Left, other.Left) && EqualityComparer<TOwner>.Default.Equals(Right, other.Right))
                || (EqualityComparer<TOwner>.Default.Equals(Left, other.Right) && EqualityComparer<TOwner>.Default.Equals(Right, other.Left));
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

            return Equals((EqualsConstraint<TOwner, TFieldType>) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (EqualityComparer<TOwner>.Default.GetHashCode(Left) ^ EqualityComparer<TOwner>.Default.GetHashCode(Right)) * 919;
            }
        }

        public TOwner Left { get; }
        public TOwner Right { get; }
        
        public EqualsConstraint(TOwner left, TOwner right)
        {
            Left = left;
            Right = right;
        }

        protected override Constraint Negation => new EqualsNotConstraint<TOwner, TFieldType>(Left, Right);
    }
}