using System;

namespace wsolve
{
    public static class RNG
    {
        private static Random rnd = new Random();

        public static float NextFloat() => (float)rnd.NextDouble();
        public static int NextInt() => rnd.Next();
        public static int NextInt(int min, int max) => rnd.Next(min, max);

        public static int[] NextInts(int number, int min, int max)
        {
            int[] x = new int[number];
            for (int i = 0; i < x.Length; i++)
            {
                x[i] = NextInt(min, max);
            }

            return x;
        }

        public static void Seed(int seed) => rnd = new Random(seed);
    }
}