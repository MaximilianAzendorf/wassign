using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

// ReSharper disable all

namespace WSolve
{
#if DEBUG
    public static class InputGenerator
    {
        public static int GenMain()
        {
            Random rnd = new Random();
            
            string ws(int i) => $"Workshop {i}";
            string hn(int i) => $"Human {i}";
            string sl(int i) => $"Slot {i}";
            
            int HumanCount, WorkshopCount, SlotCount, WorkshopOff;
            float WorkshopSizeVar;
            
            Console.WriteLine("Input: #part #ws #slot wsvar wsoff");
            float[] input = Console.ReadLine().Split(' ').Select(f => float.Parse(f, CultureInfo.InvariantCulture)).ToArray();

            HumanCount = (int) input[0];
            WorkshopCount = (int) input[1];
            SlotCount = (int) input[2];
            WorkshopSizeVar = input[3];
            WorkshopOff = (int) input[4];

            int _avgWsSize = HumanCount * SlotCount / WorkshopCount;
            int rem = HumanCount;

            List<int> conductorHumans = Enumerable.Range(0, HumanCount).ToList();

            for (int i = 0; i < SlotCount; i++)
            {
                Console.Error.WriteLine($"(slot) {sl(i)}");
            }

            int[] conductors = new int[WorkshopCount];
            
            for (int i = 0; i < WorkshopCount; i++)
            {
                int min = (int) (_avgWsSize * (1 - WorkshopSizeVar));
                int max = (int) (_avgWsSize * (1 + WorkshopSizeVar));
                rem -= max;
                if (i == WorkshopCount - 1 && rem > 0)
                {
                    max += rem;
                    min += rem;
                }

                int cond = conductorHumans[rnd.Next(conductorHumans.Count)];
                conductors[i] = cond;
                conductorHumans.Remove(cond);

                min += WorkshopOff;
                max += WorkshopOff;
                
                Console.Error.WriteLine($"(workshop) {ws(i)}: {hn(cond)}, {min}-{max}");
            }

            for (int i = 0; i < HumanCount; i++)
            {
                Console.Error.Write($"(person) {hn(i)}:");
                int favourite = rnd.Next(WorkshopCount);
                
                for (int j = 0; j < WorkshopCount; j++)
                {
                    if (conductors[j] == i)
                    {
                        Console.Error.Write(" 100");
                    }
                    else if (j == favourite)
                    {
                        Console.Error.Write(" 100");
                    }
                    else
                    {
                        Console.Error.Write($" {Math.Min(100, Math.Max(0, rnd.Next(-35, 122)))}");
                    }
                }
                
                Console.Error.WriteLine();
            }

            return 0;
        }
    }
#endif
}