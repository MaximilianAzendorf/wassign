namespace WSolve.ExtraConditions {
    public class ParticipantAccessorBase 
    {
        protected ExtraConditionsBase _base;
        protected Chromosome _chromosome;
        protected int _id;

        public ParticipantAccessorBase(int id, ExtraConditionsBase @base, Chromosome chromosome)
        {
            _id = id;
            _base = @base;
            _chromosome = chromosome;
        }

        public string Name => _chromosome.InputData.Participants[_id].name;

        public static bool operator ==(ParticipantAccessorBase left, ParticipantAccessorBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(ParticipantAccessorBase left, ParticipantAccessorBase right)
        {
            return !Equals(left, right);
        }

        public static bool operator ==(ParticipantAccessorBase left, string right)
        {
            return left == left?._base.Participant(right);
        }

        public static bool operator ==(string left, ParticipantAccessorBase right)
        {
            return right == right?._base.Participant(left);
        }

        public static bool operator !=(ParticipantAccessorBase left, string right)
        {
            return !(left == right);
        }

        public static bool operator !=(string left, ParticipantAccessorBase right)
        {
            return !(left == right);
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

            if (obj is string s)
            {
                return this == s;
            }

            if (obj.GetType() != GetType())
            {
                return false;
            }

            return Equals((ParticipantAccessorBase) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        protected bool Equals(ParticipantAccessorBase other)
        {
            return _id == other._id;
        }
    }
}