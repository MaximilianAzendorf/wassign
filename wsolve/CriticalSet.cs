using System.Collections;
using System.Collections.Generic;

namespace WSolve
{
    public class CriticalSet : IEnumerable<int>
    {
        private readonly HashSet<int> _data;

        public CriticalSet(int preference, IEnumerable<int> data)
        {
            Preference = preference;
            _data = new HashSet<int>(data);
        }

        public int Size => _data.Count;

        public int Preference { get; }

        public IEnumerator<int> GetEnumerator()
        {
            return _data.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return _data.GetEnumerator();
        }

        public bool IsCoveredBy(CriticalSet other)
        {
            return other._data.IsSubsetOf(_data) && Preference <= other.Preference;
        }

        public bool IsSubsetOf(CriticalSet other)
        {
            return _data.IsSubsetOf(other._data);
        }

        public bool Contains(int item)
        {
            return _data.Contains(item);
        }
    }
}