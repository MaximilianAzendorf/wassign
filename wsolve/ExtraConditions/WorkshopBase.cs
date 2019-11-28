using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions 
{
    public class WorkshopBase
    {
        protected readonly InputData _inputData;
        protected readonly int _id;

        public WorkshopBase(int id, InputData inputData)
        {
            _id = id;
            _inputData = inputData;
        }

        public string Name => _inputData.Workshops[_id].name;
        internal int Id => _id;
        public int MinParticipants => _inputData.Workshops[_id].min;
        public int MaxParticipants => _inputData.Workshops[_id].max;

        public static bool operator ==(WorkshopBase left, WorkshopBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(WorkshopBase left, WorkshopBase right)
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

            return Equals((WorkshopBase) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        protected bool Equals(WorkshopBase other)
        {
            return _id == other._id;
        }
    }
}