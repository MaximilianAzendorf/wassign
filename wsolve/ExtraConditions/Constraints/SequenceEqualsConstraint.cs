using System.Collections.Generic;

namespace WSolve.ExtraConditions.Constraints
{
    public class SequenceEqualsConstraint<TOwner, TCollectionElement> : Constraint
    {
        protected bool Equals(SequenceEqualsConstraint<TOwner, TCollectionElement> other)
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

            return Equals((SequenceEqualsConstraint<TOwner, TCollectionElement>) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (EqualityComparer<TOwner>.Default.GetHashCode(Left) ^ EqualityComparer<TOwner>.Default.GetHashCode(Right)) * 179;
            }
        }

        public TOwner Left { get; }
        public TOwner Right { get; }
        
        public SequenceEqualsConstraint(TOwner left, TOwner right)
        {
            Left = left;
            Right = right;
        }

        protected override Constraint Negation =>
            throw new ConstraintException("Sequence-equals constraints can not be negated.");
    }
}