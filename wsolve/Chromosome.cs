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
        private int[] array;
        private Input input;

        public int this[int index] => array[index];

        public IEnumerator<int> GetEnumerator()
        {
            return array.AsEnumerable().GetEnumerator();
        }

        public Chromosome(Input input, int[] data = null)
        {
            this.input = input;
            array = data?.ToArray() ?? new int[input.Workshops.Count + input.Participants.Count * input.Slots.Count];
            Debug.Assert(array.Length == input.Workshops.Count + input.Participants.Count * input.Slots.Count);
        }

        public Chromosome(Chromosome chromosome)
            : this(chromosome.input, chromosome.array)
        {
        }

        public ref int Slot(int workshop)
        {
            Debug.Assert(workshop < input.Workshops.Count && workshop >= 0);
            return ref array[workshop];
        }

        public ref int Workshop(int participant, int workshopNumber)
        {
            Debug.Assert(participant >= 0 && participant < input.Participants.Count);
            Debug.Assert(workshopNumber >= 0 && workshopNumber < input.Slots.Count);
            return ref array[input.Workshops.Count + participant * input.Slots.Count + workshopNumber];
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
