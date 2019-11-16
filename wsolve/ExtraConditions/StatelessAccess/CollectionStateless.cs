using WSolve.ExtraConditions.StatelessAccess.Constraints;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public class CollectionStateless<TOwner, TCollectionElement> : StatelessAccess<TOwner>
    {
        public CollectionStateless(TOwner owner) : base(owner) { }

        public Constraint Contains(TCollectionElement element) =>
            new ContainsConstraint<TOwner, TCollectionElement>(Owner, element);
    }
}