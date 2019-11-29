using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;

namespace WSolve
{
    public struct Candidate
    {
        public static readonly Candidate Null = default;

        private readonly int[] _data;

        public Candidate(Candidate candidate)
            : this(candidate.InputData, candidate._data) { }

        private Candidate(InputData inputData, int[] data = null)
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

        public static bool operator ==(Candidate left, Candidate right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(Candidate left, Candidate right)
        {
            return !left.Equals(right);
        }

        public static Candidate FromSolution(InputData inputData, Solution solution)
        {
            var c = new Candidate(inputData);

            for (int w = 0; w < inputData.Workshops.Count; w++)
            {
                c.Slot(w) = solution.Scheduling[w];
            }

            for (int p = 0; p < inputData.Participants.Count; p++)
            {
                for (int i = 0; i < inputData.Slots.Count; i++)
                {
                    c.Workshop(p, i) = solution.Assignment[p][i];
                }
            }

            return c;
        }

        [Pure]
        public ref int Slot(int workshop)
        {
            Debug.Assert(
                InputData != null,
                "Tried to access empty candidate.");
            Debug.Assert(
                workshop < InputData.Workshops.Count && workshop >= 0,
                "Tried to use out of bounds workshopNumber index.");

            return ref _data[workshop];
        }

        [Pure]
        public ref int Workshop(int participant, int workshopNumber)
        {
            Debug.Assert(
                InputData != null,
                "Tried to access empty candidate.");
            Debug.Assert(
                participant >= 0 && participant < InputData.Participants.Count,
                "Tried to use out of bounds participant index.");
            Debug.Assert(
                workshopNumber >= 0 && workshopNumber < InputData.Slots.Count,
                "Tried to use out of bounds workshopNumber index.");

            return ref _data[InputData.Workshops.Count + participant * InputData.Slots.Count + workshopNumber];
        }

        [Pure]
        public IEnumerable<int> Workshops(int participant)
        {
            for (int s = 0; s < InputData.SlotCount; s++)
            {
                yield return _data[InputData.Workshops.Count + participant * InputData.Slots.Count + s];
            }
        }

        [Pure]
        public IEnumerable<int> Participants(int workshop)
        {
            for (int p = 0; p < InputData.ParticipantCount; p++)
            {
                for (int s = 0; s < InputData.SlotCount; s++)
                {
                    if (_data[InputData.Workshops.Count + p * InputData.Slots.Count + s] == workshop)
                    {
                        yield return p;
                    }
                }
            }
        }

        public int CountParticipants(int workshop)
        {
            return _data.Skip(InputData.Workshops.Count).Count(w => w == workshop);
        }

        public Candidate Clone()
        {
            return new Candidate(this);
        }

        public Solution ToSolution()
        {
            Candidate @this = this;
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

        public bool Equals(Candidate other)
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