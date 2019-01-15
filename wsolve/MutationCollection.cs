using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace wsolve
{
    public class MutationCollection : ICollection<(float propability, IMutation mutation)>
    {
        private Dictionary<IMutation, float> mutations { get; } = new Dictionary<IMutation, float>();

        public int Count => mutations.Count;

        public bool IsReadOnly => false;

        public void Add((float propability, IMutation mutation) item)
        {
            mutations.Add(item.mutation, item.propability);
        }

        public void Add(float propability, IMutation mutation)
        {
            mutations.Add(mutation, propability);
        }

        public void Clear()
        {
            mutations.Clear();
        }

        public bool Contains((float propability, IMutation mutation) item)
        {
            return mutations.TryGetValue(item.mutation, out float p) && p == item.propability;
        }

        public bool Contains(float propability, IMutation mutation)
        {
            return mutations.TryGetValue(mutation, out float p) && p == propability;
        }
        
        public void CopyTo((float propability, IMutation mutation)[] array, int arrayIndex)
        {
            mutations.Select(kvp => (kvp.Value, kvp.Key)).ToList().CopyTo(array, arrayIndex);
        }

        public bool Remove((float propability, IMutation mutation) item)
        {
            if(!mutations.TryGetValue(item.mutation, out float p) || p != item.propability)
            {
                return false;
            }

            return mutations.Remove(item.mutation);
        }

        public bool Remove(IMutation mutation)
        {
            return mutations.Remove(mutation);
        }

        public IEnumerator<(float propability, IMutation mutation)> GetEnumerator()
        {
            return mutations.Select(kvp => (kvp.Value, kvp.Key)).GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
