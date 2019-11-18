namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public sealed class ContainsNotConstraint<TOwner, TCollectionElement> : Constraint
    {
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