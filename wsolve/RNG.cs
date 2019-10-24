namespace WSolve
{
    using System;

    public static class RNG
    {
        [ThreadStatic]
        private static Random _rnd;
        
        private static Random Rnd => _rnd ?? (_rnd = new Random());

        public static float NextFloat() => (float)Rnd.NextDouble();
        
        public static int NextInt() => Rnd.Next();
        
        public static int NextInt(int min, int max) => Rnd.Next(min, max);

        public static int[] NextInts(int number, int min, int max)
        {
            int[] x = new int[number];
            for (int i = 0; i < x.Length; i++)
            {
                x[i] = NextInt(min, max);
            }

            return x;
        }
    }
}