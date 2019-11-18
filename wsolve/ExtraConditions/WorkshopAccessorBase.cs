using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions {
    public class WorkshopAccessorBase 
    {
        protected ExtraConditionsBase _base;
        protected Chromosome Chromosome;
        protected int _id;

        public WorkshopAccessorBase(int id, ExtraConditionsBase @base, Chromosome chromosome)
        {
            _id = id;
            _base = @base;
            Chromosome = chromosome;
        }

        public string Name => Chromosome.InputData.Workshops[_id].name;
        internal int Id => _id;
        public int MinParticipants => Chromosome.InputData.Workshops[_id].min;
        public int MaxParticipants => Chromosome.InputData.Workshops[_id].max;

        public static bool operator ==(WorkshopAccessorBase left, WorkshopAccessorBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(WorkshopAccessorBase left, WorkshopAccessorBase right)
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