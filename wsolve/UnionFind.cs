using System;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class UnionFind<T>
    {
        private readonly Dictionary<T, int> _elements;
        private readonly int[] _parents;
        private readonly int[] _heights;
        
        public UnionFind(IEnumerable<T> elements)
        {
            _elements = elements.Select((e, i) => (e, i)).ToDictionary(x => x.e, x => x.i);
            _parents = new int[_elements.Count];
            _heights = new int[_elements.Count];

            Array.Fill(_parents, -1);
            Array.Fill(_heights, 0);
        }

        public int Find(T element)
        {
            int idx = _elements[element];
            while (_parents[idx] != -1)
            {
                idx = _parents[idx];
            }

            return idx;
        }

        public int Union(T e1, T e2)
        {
            int idx1 = Find(e1);
            int idx2 = Find(e2);

            if(idx1 == idx2) return idx1;

            if (_heights[idx1] > _heights[idx2])
            {
                (idx2, idx1) = (idx1, idx2);
            }

            _parents[idx1] = idx2;
            _heights[idx2] = Math.Max(_heights[idx2], _heights[idx1] + 1);
            
            return idx2;
        }
    }
}