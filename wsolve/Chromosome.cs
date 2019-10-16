using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Diagnostics;
using System.Text;

namespace WSolve
{
    public struct Chromosome : IEnumerable<int>
    {
        public static readonly Chromosome Null = new Chromosome();
        
        private readonly int[] _array;
        private readonly InputData _inputData;

        public int Length => _inputData.Workshops.Count + _inputData.Participants.Count * _inputData.Slots.Count;

        public InputData InputData => _inputData;
        
        public ref int this[int index] => ref _array[index];

        public IEnumerator<int> GetEnumerator()
        {
            return _array.AsEnumerable().GetEnumerator();
        }
        
        public Chromosome(InputData inputData, int[] data = null)
        {
            _inputData = inputData;
            _array = data?.ToArray() ?? new int[inputData.Workshops.Count + inputData.Participants.Count * inputData.Slots.Count];
            Debug.Assert(_array.Length == inputData.Workshops.Count + inputData.Participants.Count * inputData.Slots.Count);
        }

        public Chromosome(Chromosome chromosome)
            : this(chromosome._inputData, chromosome._array)
        {
        }

        public int GenerateWorkshopGene() => RNG.NextInt(0, _inputData.Workshops.Count);
        public int GenerateSlotGene() => RNG.NextInt(0, _inputData.Slots.Count);
        
        public ref int Slot(int workshop)
        {
            Debug.Assert(_inputData != null);
            Debug.Assert(workshop < _inputData.Workshops.Count && workshop >= 0);
            return ref _array[workshop];
        }

        public ref int Workshop(int participant, int workshopNumber)
        {
            Debug.Assert(_inputData != null);
            Debug.Assert(participant >= 0 && participant < _inputData.Participants.Count);
            Debug.Assert(workshopNumber >= 0 && workshopNumber < _inputData.Slots.Count);
            return ref _array[_inputData.Workshops.Count + participant * _inputData.Slots.Count + workshopNumber];
        }
        
        public int CountParticipants(int workshop) => _array.Skip(_inputData.Workshops.Count).Count(w => w == workshop);

        public int CountPreference(int pref)
        {
            int c = 0;
            for (int p = 0; p < InputData.Participants.Count; p++)
            {
                for (int s = 0; s < InputData.Slots.Count; s++)
                {
                    if (InputData.Participants[p].preferences[Workshop(p, s)] == pref)
                    {
                        c++;
                    }
                }
            }

            return c;
        }

        public int MaxUsedPreference
        {
            get
            {
                int c = int.MinValue;
                for (int p = 0; p < InputData.Participants.Count; p++)
                {
                    for (int s = 0; s < InputData.Slots.Count; s++)
                    {
                        c = Math.Max(InputData.Participants[p].preferences[Workshop(p, s)], c);
                    }
                }

                return c;
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
        
        public Chromosome Copy() => new Chromosome(this);

        public static Chromosome FromOutput(InputData inputData, Solution solution)
        {
            Chromosome c = new Chromosome(inputData);

            for (int i = 0; i < inputData.Workshops.Count; i++)
            {
                c.Slot(i) = solution.Scheduling[i];
            }

            for (int i = 0; i < inputData.Participants.Count; i++)
            {
                for (int j = 0; j < inputData.Slots.Count; j++)
                {
                    c.Workshop(i, j) = solution.Assignment[i][j];
                }
            }

            return c;
        }

        public float Distance(Chromosome other)
        {
            int h = 0;
            for (int i = 0; i < Length; i++)
            {
                if (_array[i] != other._array[i])
                    h++;
            }

            return h / (float) Length;
        }

        public override bool Equals(object obj)
        {
            return base.Equals(obj);
        }

        public bool Equals(Chromosome other)
        {
            if (_array == null) return other._array == null;
            if (other._array == null) return false;
            if (_array.Length != other._array.Length) return false;
            if (_inputData != other._inputData) return false;
            for(int i = 0; i < _array.Length; i++)
                if (_array[i] != other._array[i])
                    return false;
            return true;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int arr = 0;
                for (int i = 0; i < _array.Length; i++)
                {
                    arr = arr * 101 + _array[i].GetHashCode();
                }
                return arr * 397 ^ (_inputData != null ? _inputData.GetHashCode() : 0);
            }
        }

        public static bool operator ==(Chromosome left, Chromosome right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(Chromosome left, Chromosome right)
        {
            return !left.Equals(right);
        }
    }
}
