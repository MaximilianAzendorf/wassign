using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;

namespace WSolve
{
    public struct Chromosome : IEnumerable<int>
    {
        public static readonly Chromosome Null = default;

        private readonly int[] _data;

        public Chromosome(Chromosome chromosome)
            : this(chromosome.InputData, chromosome._data) { }

        private Chromosome(InputData inputData, int[] data = null)
        {
            InputData = inputData;
            _data = data?.ToArray() ??
                    new int[inputData.Workshops.Count + inputData.Participants.Count * inputData.Slots.Count];
        }

        public int Length => InputData.Workshops.Count + InputData.Participants.Count * InputData.Slots.Count;

        public InputData InputData { get; }

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

        public static bool operator ==(Chromosome left, Chromosome right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(Chromosome left, Chromosome right)
        {
            return !left.Equals(right);
        }

        public static Chromosome FromOutput(InputData inputData, Solution solution)
        {
            var c = new Chromosome(inputData);

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

        [Pure]
        public ref int Slot(int workshop)
        {
            Debug.Assert(
                InputData != null,
                "Tried to access empty chromosome.");
            Debug.Assert(
                workshop < InputData.Workshops.Count && workshop >= 0,
                "Tried to use out of bounds workshop index.");

            return ref _data[workshop];
        }

        [Pure]
        public ref int Workshop(int participant, int workshop)
        {
            Debug.Assert(
                InputData != null,
                "Tried to access empty chromosome.");
            Debug.Assert(
                participant >= 0 && participant < InputData.Participants.Count,
                "Tried to use out of bounds participant index.");
            Debug.Assert(
                workshop >= 0 && workshop < InputData.Slots.Count,
                "Tried to use out of bounds workshop index.");

            return ref _data[InputData.Workshops.Count + participant * InputData.Slots.Count + workshop];
        }

        public int CountParticipants(int workshop)
        {
            return _data.Skip(InputData.Workshops.Count).Count(w => w == workshop);
        }

        public IEnumerator<int> GetEnumerator()
        {
            return _data.AsEnumerable().GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public Chromosome Clone()
        {
            return new Chromosome(this);
        }

        public Solution ToSolution()
        {
            Chromosome @this = this;
            return new Solution(
                InputData,
                Enumerable.Range(0, InputData.Workshops.Count).Select(w => (w, @this.Slot(w))),
                Enumerable.Range(0, InputData.Participants.Count).SelectMany(p =>
                    Enumerable.Range(0, @this.InputData.Slots.Count).Select(s => (p, @this.Workshop(p, s)))));
        }

        public override bool Equals(object obj)
        {
            return base.Equals(obj);
        }

        public bool Equals(Chromosome other)
        {
            if (_data == null)
            {
                return other._data == null;
            }

            if (other._data == null)
            {
                return false;
            }

            if (_data.Length != other._data.Length)
            {
                return false;
            }

            if (InputData != other.InputData)
            {
                return false;
            }

            for (int i = 0; i < _data.Length; i++)
            {
                if (_data[i] != other._data[i])
                {
                    return false;
                }
            }

            return true;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int arr = 0;
                for (int i = 0; i < _data.Length; i++)
                {
                    arr = arr * 101 + _data[i].GetHashCode();
                }

                return (arr * 397) ^ (InputData != null ? InputData.GetHashCode() : 0);
            }
        }
    }
}