namespace WSolve.ExtraConditions 
{
    public class SlotAccessorBase 
    {
        protected ExtraConditionsBase _base;
        protected Chromosome Chromosome;
        protected int _id;

        public SlotAccessorBase(int id, ExtraConditionsBase @base, Chromosome chromosome)
        {
            _id = id;
            _base = @base;
            Chromosome = chromosome;
        }

        public string Name => Chromosome.InputData.Slots[_id];
        internal int Id => _id;

        public static bool operator ==(SlotAccessorBase left, SlotAccessorBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(SlotAccessorBase left, SlotAccessorBase right)
        {
            return !Equals(left, right);
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

            if (obj.GetType() != GetType())
            {
                return false;
            }

            return Equals((SlotAccessorBase) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        private bool Equals(SlotAccessorBase other)
        {
            return _id == other._id;
        }
    }
}