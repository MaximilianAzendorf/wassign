using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public class Scheduling : IReadOnlyDictionary<int, int>, IEnumerable<(int workshop, int slot)>
    {
        private readonly Dictionary<int, int> _dictionary;
        private readonly InputData _inputData;

        public Scheduling(Scheduling scheduling)
            : this(scheduling._inputData, scheduling._dictionary)
        {
        }
        
        public Scheduling(InputData inputData, IEnumerable<(int workshop, int slot)> scheduling)
            : this(inputData, scheduling.ToDictionary(s => s.workshop, s => s.slot))
        {
        }

        public Scheduling(InputData inputData, IReadOnlyDictionary<int, int> scheduling)
        {
            _dictionary = new Dictionary<int, int>(scheduling);
            _inputData = inputData;

            if (_dictionary.Keys.Count != inputData.WorkshopCount ||
                _dictionary.Keys.Any(k => k < 0 || k >= inputData.WorkshopCount) ||
                _dictionary.Values.Any(v => v < 0 || v >= inputData.SlotCount))
            {
                throw new ArgumentException("Given data is not a valid scheduling");
            }
        }

        public bool IsFeasible()
        {
            int[] slotMin = new int[_inputData.SlotCount];
            int[] slotMax = new int[_inputData.SlotCount];

            foreach (var s in this)
            {
                slotMin[s.slot] += _inputData.Workshops[s.workshop].min;
                slotMax[s.slot] += _inputData.Workshops[s.workshop].max;
            }

            return slotMin.All(m => m <= _inputData.ParticipantCount) && 
                   slotMax.All(m => m >= _inputData.ParticipantCount);
        }
        
        public IEnumerable<(int workshop, int slot)> AsEnumerable() => this;

        IEnumerator<KeyValuePair<int, int>> IEnumerable<KeyValuePair<int, int>>.GetEnumerator()
        {
            return _dictionary.GetEnumerator();
        }

        public IEnumerator<(int workshop, int slot)> GetEnumerator()
        {
            return _dictionary.Select(kvp => (kvp.Key, kvp.Value)).GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable) _dictionary).GetEnumerator();
        }

        public int Count => _dictionary.Count;

        public bool ContainsKey(int key)
        {
            return _dictionary.ContainsKey(key);
        }

        public bool TryGetValue(int key, out int value)
        {
            return _dictionary.TryGetValue(key, out value);
        }

        public int this[int key]
        {
            get => _dictionary[key];
            set
            {
                if (value < 0 || value >= _inputData.SlotCount)
                {
                    throw new ArgumentOutOfRangeException(nameof(value));
                }
                _dictionary[key] = value;
            }
        }

        public IEnumerable<int> Keys => _dictionary.Keys;

        public IEnumerable<int> Values => _dictionary.Values;

        public override bool Equals(object obj)
        {
            Scheduling sched = obj as Scheduling;
            if(sched == null) return false;

            if(sched._inputData != _inputData) return false;
            
            foreach (var kvp in _dictionary)
            {
                if (sched._dictionary[kvp.Key] != kvp.Value)
                {
                    return false;
                }
            }
            
            return true;
        }

        public override int GetHashCode()
        {
            int hash = _inputData.GetHashCode();

            for (int i = 0; i < _inputData.WorkshopCount; i++)
            {
                hash = (hash * 397) ^ _dictionary[i];
            }

            return hash;
        }
    }
}