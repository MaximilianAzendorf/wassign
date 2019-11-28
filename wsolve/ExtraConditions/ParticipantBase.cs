namespace WSolve.ExtraConditions
{
    public class ParticipantBase 
    {
        protected readonly int _id;
        protected readonly InputData _inputData;

        public ParticipantBase(int id, InputData inputData)
        {
            _id = id;
            _inputData = inputData;
        }

        public string Name => _inputData.Participants[_id].name;
        internal int Id => _id;

        public static bool operator ==(ParticipantBase left, ParticipantBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(ParticipantBase left, ParticipantBase right)
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

            return Equals((ParticipantBase) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        protected bool Equals(ParticipantBase other)
        {
            return _id == other._id;
        }
    }
}