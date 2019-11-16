namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public sealed class ContainsConstraint<TOwner, TCollectionElement> : Constraint
    {
        public TOwner Owner { get; }
        public TCollectionElement Element { get; }
        
        public ContainsConstraint(TOwner owner, TCollectionElement element)
        {
            Owner = owner;
            Element = element;
        }
    }
}