using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace WSolve.ExtraConditions 
{
    public class SlotAccessor
    {
        protected ExtraConditionsBase _base;
        protected Chromosome _chromosome;
        protected int _id;

        public SlotAccessor(int id, ExtraConditionsBase @base, Chromosome chromosome)
        {
            _id = id;
            _base = @base;
            _chromosome = chromosome;
        }

        public string Name => _chromosome.InputData.Slots[_id];

        public static bool operator ==(SlotAccessor left, SlotAccessor right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(SlotAccessor left, SlotAccessor right)
        {
            return !Equals(left, right);
        }

        public static bool operator ==(SlotAccessor left, string right)
        {
            return left == left?._base.Slot(right);
        }

        public static bool operator ==(string left, SlotAccessor right)
        {
            return right == right?._base.Slot(left);
        }

        public static bool operator !=(SlotAccessor left, string right)
        {
            return !(left == right);
        }

        public static bool operator !=(string left, SlotAccessor right)
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

            return Equals((SlotAccessor) obj);
        }

        public override int GetHashCode()
        {
            return _id;
        }

        private bool Equals(SlotAccessor other)
        {
            return _id == other._id;
        }
        

        public IReadOnlyCollection<WorkshopAccessor> Workshops => Enumerable
            .Range(0, _chromosome.InputData.Workshops.Count)
            .Where(w => _chromosome.Slot(w) == _id)
            .Select(w => new WorkshopAccessor(w, _base, _chromosome))
            .ToImmutableList();
    }
}