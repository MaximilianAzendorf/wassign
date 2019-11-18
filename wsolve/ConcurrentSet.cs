using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;

namespace WSolve
{
    public class ConcurrentSet<T>
    {
        private readonly ConcurrentDictionary<T, byte> _data = new ConcurrentDictionary<T, byte>();

        public void Add(T item)
        {
            _data.AddOrUpdate(item, 0, (unused0, unused1) => 0);
        }

        public bool Contains(T item)
        {
            return _data.ContainsKey(item);
        }

    }
}