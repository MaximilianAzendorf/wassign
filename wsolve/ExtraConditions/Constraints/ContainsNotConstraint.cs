using System.Collections.Generic;

namespace WSolve.ExtraConditions.Constraints
{
    public sealed class ContainsNotConstraint<TOwner, TCollectionElement> : Constraint
    {
        private bool Equals(ContainsNotConstraint<TOwner, TCollectionElement> other)
        {
            return EqualityComparer<TOwner>.Default.Equals(Owner, other.Owner) && EqualityComparer<TCollectionElement>.Default.Equals(Element, other.Element);
        }

        public override bool Equals(object obj)
        {
            return ReferenceEquals(this, obj) || obj is ContainsNotConstraint<TOwner, TCollectionElement> other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((EqualityComparer<TOwner>.Default.GetHashCode(Owner) * 397) ^ EqualityComparer<TCollectionElement>.Default.GetHashCode(Element)) * 467;
            }
        }

        public TOwner Owner { get; }
        public TCollectionElement Element { get; }
        
        public ContainsNotConstraint(TOwner owner, TCollectionElement element)
        {
            Owner = owner;
            Element = element;
        }

        protected override Constraint Negation => new ContainsConstraint<TOwner, TCollectionElement>(Owner, Element);
    }
}