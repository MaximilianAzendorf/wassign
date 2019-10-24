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

        private readonly int[] _array;

        public Chromosome(Chromosome chromosome)
            : this(chromosome.InputData, chromosome._array)
        {
        }

        private Chromosome(InputData inputData, int[] data = null)
        {
            InputData = inputData;
            _array = data?.ToArray() ??
                     new int[inputData.Workshops.Count + inputData.Participants.Count * inputData.Slots.Count];
        }

        public int Length => InputData.Workshops.Count + InputData.Participants.Count * InputData.Slots.Count;

        public InputData InputData { get; }

        public int MaxUsedPreference
        {
            get
            {
                var c = int.MinValue;
                for (var p = 0; p < InputData.Participants.Count; p++)
                for (var s = 0; s < InputData.Slots.Count; s++)
                    c = Math.Max(InputData.Participants[p].preferences[Workshop(p, s)], c);

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

            for (var i = 0; i < inputData.Workshops.Count; i++) c.Slot(i) = solution.Scheduling[i];

            for (var i = 0; i < inputData.Participants.Count; i++)
            for (var j = 0; j < inputData.Slots.Count; j++)
                c.Workshop(i, j) = solution.Assignment[i][j];

            return c;
        }

        public int GenerateWorkshopGene()
        {
            return RNG.NextInt(0, InputData.Workshops.Count);
        }

        public int GenerateSlotGene()
        {
            return RNG.NextInt(0, InputData.Slots.Count);
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

            return ref _array[workshop];
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

            return ref _array[InputData.Workshops.Count + participant * InputData.Slots.Count + workshop];
        }

        public int CountParticipants(int workshop)
        {
            return _array.Skip(InputData.Workshops.Count).Count(w => w == workshop);
        }

        public IEnumerator<int> GetEnumerator()
        {
            return _array.AsEnumerable().GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public Chromosome Copy()
        {
            return new Chromosome(this);
        }

        public Solution ToSolution()
        {
            var @this = this;
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
            if (_array == null) return other._array == null;

            if (other._array == null) return false;

            if (_array.Length != other._array.Length) return false;

            if (InputData != other.InputData) return false;

            for (var i = 0; i < _array.Length; i++)
                if (_array[i] != other._array[i])
                    return false;

            return true;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var arr = 0;
                for (var i = 0; i < _array.Length; i++) arr = arr * 101 + _array[i].GetHashCode();

                return (arr * 397) ^ (InputData != null ? InputData.GetHashCode() : 0);
            }
        }
    }
}