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
        private static uint nextId = 1;

        public uint Id { get; }
        
        private readonly int[] _array;
        private readonly Input _input;

        public int Length => _input.Workshops.Count + _input.Participants.Count * _input.Slots.Count;

        public ref int this[int index] => ref _array[index];

        public IEnumerator<int> GetEnumerator()
        {
            return _array.AsEnumerable().GetEnumerator();
        }

        public Chromosome(Input input, int[] data = null)
        {
            Id = nextId++;
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

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

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
    }
}
