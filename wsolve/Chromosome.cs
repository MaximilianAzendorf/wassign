using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Diagnostics;
using System.Text;

namespace wsolve
{
    public struct Chromosome : IEnumerable<int>
    {
        public static readonly Chromosome Null = new Chromosome();
        
        private readonly int[] _array;
        private readonly Input _input;

        public int Length => _input.Workshops.Count + _input.Participants.Count * _input.Slots.Count;

        public Input Input => _input;
        
        public ref int this[int index] => ref _array[index];

        public IEnumerator<int> GetEnumerator()
        {
            return _array.AsEnumerable().GetEnumerator();
        }
        
        public Chromosome(Input input, int[] data = null)
        {
            _input = input;
            _array = data?.ToArray() ?? new int[input.Workshops.Count + input.Participants.Count * input.Slots.Count];
            Debug.Assert(_array.Length == input.Workshops.Count + input.Participants.Count * input.Slots.Count);
        }

        public Chromosome(Chromosome chromosome)
            : this(chromosome._input, chromosome._array)
        {
        }

        public int GenerateWorkshopGene() => RNG.NextInt(0, _input.Workshops.Count);
        public int GenerateSlotGene() => RNG.NextInt(0, _input.Slots.Count);

        public ref int Slot(int workshop)
        {
            Debug.Assert(_input != null);
            Debug.Assert(workshop < _input.Workshops.Count && workshop >= 0);
            return ref _array[workshop];
        }

        public ref int Workshop(int participant, int workshopNumber)
        {
            Debug.Assert(_input != null);
            Debug.Assert(participant >= 0 && participant < _input.Participants.Count);
            Debug.Assert(workshopNumber >= 0 && workshopNumber < _input.Slots.Count);
            return ref _array[_input.Workshops.Count + participant * _input.Slots.Count + workshopNumber];
        }

        public int CountParticipants(int workshop) => _array.Skip(_input.Workshops.Count).Count(w => w == workshop);

        public int CountPreference(int pref)
        {
            int c = 0;
            for (int p = 0; p < Input.Participants.Count; p++)
            {
                for (int s = 0; s < Input.Slots.Count; s++)
                {
                    if (Input.Participants[p].preferences[Workshop(p, s)] == pref)
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
                for (int p = 0; p < Input.Participants.Count; p++)
                {
                    for (int s = 0; s < Input.Slots.Count; s++)
                    {
                        c = Math.Max(Input.Participants[p].preferences[Workshop(p, s)], c);
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

        public static Chromosome FromOutput(Input input, Output output)
        {
            Chromosome c = new Chromosome(input);

            for (int i = 0; i < input.Workshops.Count; i++)
            {
                c.Slot(i) = output.Scheduling[i];
            }

            for (int i = 0; i < input.Participants.Count; i++)
            {
                for (int j = 0; j < input.Slots.Count; j++)
                {
                    c.Workshop(i, j) = output.Assignment[i][j];
                }
            }

            return c;
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
            if (_input != other._input) return false;
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
                return arr * 397 ^ (_input != null ? _input.GetHashCode() : 0);
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
