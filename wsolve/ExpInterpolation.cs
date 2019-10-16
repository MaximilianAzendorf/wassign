using System;

namespace WSolve
{
    public class ExpInterpolation
    {
        public readonly double From;
        public readonly double To;
        public readonly double Exp;
        
        public ExpInterpolation(double from, double to, double exp)
        {
            From = from;
            To = to;
            Exp = exp;
        }

        private double Clamp(double f) => Math.Max(0, Math.Min(1, f));
        
        public double GetValue(double f) => (To - From) * Math.Pow(Clamp(f), Exp) + From;

        public override string ToString()
        {
            if (From == To) return From.ToString();
            if (Exp == 1.0) return $"{From}-{To}";
            return $"{From}-{To}^{Exp}";
        }
    }
}