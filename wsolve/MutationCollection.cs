using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WSolve
{
    public class MutationCollection : ICollection<(float propability, IMutation mutation)>
    {
        private readonly Dictionary<IMutation, float> _mutations = new Dictionary<IMutation, float>();

        public int Count => _mutations.Count;

        public bool IsReadOnly => false;

        public void Add((float propability, IMutation mutation) item)
        {
            _mutations.Add(item.mutation, item.propability);
        }

        public void Add(float propability, IMutation mutation)
        {
            _mutations.Add(mutation, propability);
        }

        public void Clear()
        {
            _mutations.Clear();
        }

        public bool Contains((float propability, IMutation mutation) item)
        {
            return _mutations.TryGetValue(item.mutation, out float p) && p == item.propability;
        }

        public bool Contains(float propability, IMutation mutation)
        {
            return _mutations.TryGetValue(mutation, out float p) && p == propability;
        }
        
        public void CopyTo((float propability, IMutation mutation)[] array, int arrayIndex)
        {
            _mutations.Select(kvp => (kvp.Value, kvp.Key)).ToList().CopyTo(array, arrayIndex);
        }

        public bool Remove((float propability, IMutation mutation) item)
        {
            if(!_mutations.TryGetValue(item.mutation, out float p) || p != item.propability)
            {
                return false;
            }

            return _mutations.Remove(item.mutation);
        }

        public bool Remove(IMutation mutation)
        {
            return _mutations.Remove(mutation);
        }

        public (float cost, IMutation mutation)[] GetSelectionSnapshot()
        {
            float psum = _mutations.Sum(kvp => kvp.Value);

            List<(float, IMutation)> snapshot = new List<(float, IMutation)>(_mutations.Count);

            foreach ((IMutation key, float value) in _mutations.OrderByDescending(kvp => kvp.Value))
            {
                snapshot.Add((value / psum, key));
            }

            return snapshot.ToArray();
        }

        public IEnumerator<(float propability, IMutation mutation)> GetEnumerator()
        {
            return _mutations.Select(kvp => (kvp.Value, kvp.Key)).GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
