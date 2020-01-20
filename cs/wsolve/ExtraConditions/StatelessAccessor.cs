namespace WSolve.ExtraConditions
{
    public abstract class StatelessAccessor<TOwner>
    {
        public TOwner Owner { get; }
        
        protected StatelessAccessor(TOwner owner)
        {
            Owner = owner;
        }
    }
}