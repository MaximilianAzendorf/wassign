using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions {
    public class WorkshopAccessorBase 
    {
        protected ExtraConditionsBase _base;
        protected Chromosome _chromosome;
        protected int _id;

        public WorkshopAccessorBase(int id, ExtraConditionsBase @base, Chromosome chromosome)
        {
            _id = id;
            _base = @base;
            _chromosome = chromosome;
        }

        public string Name => _chromosome.InputData.Workshops[_id].name;
        public int MinParticipants => _chromosome.InputData.Workshops[_id].min;
        public int MaxParticipants => _chromosome.InputData.Workshops[_id].max;

        public SlotAccessor Slot => new SlotAccessor(_chromosome.Slot(_id), _base, _chromosome);

        public static bool operator ==(WorkshopAccessorBase left, WorkshopAccessorBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(WorkshopAccessorBase left, WorkshopAccessorBase right)
        {
            return !Equals(left, right);
        }

        public static bool operator ==(WorkshopAccessorBase left, string right)
        {
            return left == left?._base.Workshop(right);
        }

        public static bool operator ==(string left, WorkshopAccessorBase right)
        {
            return right == right?._base.Workshop(left);
        }

        public static bool operator !=(WorkshopAccessorBase left, string right)
        {
            return !(left == right);
        }

        public static bool operator !=(string left, WorkshopAccessorBase right)
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

            return Equals((WorkshopAccessorBase) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        protected bool Equals(WorkshopAccessorBase other)
        {
            return _id == other._id;
        }
    }
}