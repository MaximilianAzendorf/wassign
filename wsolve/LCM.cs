namespace WSolve
{
    public static class LCM
    {
        private static int GCD(int a, int b) => b == 0 ? a : GCD(b, a % b);
        
        public static int Get(int a, int b) => (a / GCD(a, b)) * b;
    }
}