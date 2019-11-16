namespace WSolve.ExtraConditions.StatelessAccess
{
    public abstract class StatelessAccess<TOwner>
    {
        public TOwner Owner { get; }
        
        protected StatelessAccess(TOwner owner)
        {
            Owner = owner;
        }
    }
}