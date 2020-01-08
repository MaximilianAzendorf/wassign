namespace WSolve.ExtraConditions 
{
    public class SlotBase
    {
        protected readonly int _id;
        protected readonly InputData _inputData;

        public SlotBase(int id, InputData inputData)
        {
            _id = id;
            _inputData = inputData;
        }

        public string Name => _inputData.Slots[_id];
        internal int Id => _id;

        public static bool operator ==(SlotBase left, SlotBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(SlotBase left, SlotBase right)
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

            return Equals((SlotBase) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        private bool Equals(SlotBase other)
        {
            return _id == other._id;
        }
    }
}