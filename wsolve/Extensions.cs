using System;

namespace WSolve
{
    public static class Extensions
    {
        public static string WithoutMilliseconds(this TimeSpan timeSpan)
        {
            return $"{timeSpan.Hours:D2}:{timeSpan.Minutes:D2}:{timeSpan.Seconds:D2}";
        }
    }
}