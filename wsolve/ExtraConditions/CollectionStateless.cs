using System;
using WSolve.ExtraConditions.Constraints;

namespace WSolve.ExtraConditions
{
    public class CollectionStateless<TOwner, TCollectionElement> : StatelessAccessor<TOwner>
    {
        public CollectionStateless(TOwner owner) : base(owner) { }

        public Constraint Contains(TCollectionElement element) =>
            new ContainsConstraint<TOwner, TCollectionElement>(Owner, element);

        public Constraint SequenceEqual(CollectionStateless<TOwner, TCollectionElement> other) =>
            new SequenceEqualsConstraint<TOwner, TCollectionElement>(Owner, other.Owner);

        public override bool Equals(object obj) => throw new InvalidStatelessOperationException();
        public override int GetHashCode() => throw new InvalidStatelessOperationException();
        
        public static Constraint operator ==(CollectionStateless<TOwner, TCollectionElement> left,
            CollectionStateless<TOwner, TCollectionElement> right)
            => left.SequenceEqual(right);
        
        public static Constraint operator !=(CollectionStateless<TOwner, TCollectionElement> left,
            CollectionStateless<TOwner, TCollectionElement> right)
            => !left.SequenceEqual(right);
    }
}