using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions 
{
    public class WorkshopBase
    {
        internal readonly InputData InputData;
        internal readonly int Id;

        public WorkshopBase(int id, InputData inputData)
        {
            Id = id;
            InputData = inputData;
        }

        public string Name => InputData.Workshops[Id].name;
        public int MinParticipants => InputData.Workshops[Id].min;
        public int MaxParticipants => InputData.Workshops[Id].max;

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
            return Id;
        }

        protected bool Equals(WorkshopBase other)
        {
            return Id == other.Id;
        }
    }
}