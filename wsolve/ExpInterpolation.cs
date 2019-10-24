using System;

namespace WSolve
{
    public class ExpInterpolation
    {
        public ExpInterpolation(double from, double to, double exp)
        {
            From = from;
            To = to;
            Exp = exp;
        }

        public double From { get; }

        public double To { get; }

        public double Exp { get; }

        public double GetValue(double f)
        {
            return (To - From) * Math.Pow(Clamp(f), Exp) + From;
        }

        public override string ToString()
        {
            return From == To ? From.ToString()
                : Exp == 1.0 ? $"{From}-{To}"
                : $"{From}-{To}^{Exp}";
        }

        private double Clamp(double f)
        {
            return Math.Max(0, Math.Min(1, f));
        }
    }
}