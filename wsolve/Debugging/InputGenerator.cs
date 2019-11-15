// ReSharper disable all

namespace WSolve.Debugging
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;

#if DEBUG
    internal static class InputGenerator
    {
        public static int GenMain()
        {
            Random rnd = new Random();

            string ws(int i) => $"Workshop {i}";
            string hn(int i) => $"Human {i}";
            string sl(int i) => $"Slot {i}";

            int humanCount, workshopCount, slotCount, workshopOff;
            float workshopSizeVar;

            Console.WriteLine("Input: #part #ws #slot wsvar wsoff");
            float[] input = Console.ReadLine().Split(' ').Select(f => float.Parse(f, CultureInfo.InvariantCulture))
                .ToArray();

            humanCount = (int) input[0];
            workshopCount = (int) input[1];
            slotCount = (int) input[2];
            workshopSizeVar = input[3];
            workshopOff = (int) input[4];

            int avgWsSize = humanCount * slotCount / workshopCount;
            int rem = humanCount;

            List<int> conductorHumans = Enumerable.Range(0, humanCount).ToList();

            for (int i = 0; i < slotCount; i++)
            {
                Console.Error.WriteLine($"(slot) {sl(i)}");
            }

            int[] conductors = new int[workshopCount];

            for (int i = 0; i < workshopCount; i++)
            {
                int min = (int) (avgWsSize * (1 - workshopSizeVar));
                int max = (int) (avgWsSize * (1 + workshopSizeVar));
                rem -= max;
                if (i == workshopCount - 1 && rem > 0)
                {
                    max += rem;
                    min += rem;
                }

                int cond = conductorHumans[rnd.Next(conductorHumans.Count)];
                conductors[i] = cond;
                conductorHumans.Remove(cond);

                min += workshopOff;
                max += workshopOff;

                Console.Error.WriteLine($"(workshop) {ws(i)}: {hn(cond)}, {min}-{max}");
            }

            for (int i = 0; i < humanCount; i++)
            {
                Console.Error.Write($"(person) {hn(i)}:");
                int favourite = rnd.Next(workshopCount);

                for (int j = 0; j < workshopCount; j++)
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