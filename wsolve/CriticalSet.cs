namespace WSolve
{
    using System.Collections;
    using System.Collections.Generic;

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
        
        public bool IsCoveredBy(CriticalSet other)
        {
            return other._data.IsSubsetOf(_data) && Preference <= other.Preference;
        }

        public bool IsSubsetOf(CriticalSet other)
        {
            return _data.IsSubsetOf(other._data);
        }

        public bool Contains(int item) => _data.Contains(item);
        
        public IEnumerator<int> GetEnumerator() => _data.GetEnumerator();

        IEnumerator IEnumerable.GetEnumerator() => _data.GetEnumerator();
    }
}