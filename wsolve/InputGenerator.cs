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
            
            int HumanCount, WorkshopCount, SlotCount, SkewDiv, WorkshopOff;
            float WorkshopSizeVar, SkewDivF;
            
            Console.WriteLine("Input: #part #ws #slot wsvar wsoff skew");
            float[] input = Console.ReadLine().Split(' ').Select(f => float.Parse(f, CultureInfo.InvariantCulture)).ToArray();

            HumanCount = (int) input[0];
            WorkshopCount = (int) input[1];
            SlotCount = (int) input[2];
            WorkshopSizeVar = input[3];
            WorkshopOff = (int) input[4];
            SkewDivF = input[5];

            SkewDiv = SkewDivF == 0 ? int.MaxValue : (int)(WorkshopCount / SkewDivF);

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
                
                List<int> pref = new List<int>();
                for (int j = 0; j < WorkshopCount; j++)
                {
                    for (int k = 0; k < 1 + j / SkewDiv; k++)
                    {
                        pref.Add(j);
                    }
                }
                
                pref = pref.OrderBy(n => rnd.Next()).ToList();

                bool[] encountered = new bool[WorkshopCount];
                int off = 0;
                for (int j = 0; j < WorkshopCount; j++)
                {
                    if (encountered[pref[j + off]])
                    {
                        j--;
                        off++;
                        continue;
                    }

                    if (conductors[j] == i)
                    {
                        Console.Error.Write(" 0");
                    }
                    else
                    {
                        Console.Error.Write($" {pref[j + off]}");
                    }

                    encountered[pref[j + off]] = true;
                }
                
                Console.Error.WriteLine();
            }

            return 0;
        }
    }
#endif
}