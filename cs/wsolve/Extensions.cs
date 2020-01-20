using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WSolve
{
    public static class Extensions
    {
        public static string ToStringNoMilliseconds(this TimeSpan timeSpan)
        {
            return $"{timeSpan.Hours:D2}:{timeSpan.Minutes:D2}:{timeSpan.Seconds:D2}";
        }

        public static int FindIndex<T>(this IReadOnlyList<T> list, Func<T, bool> predicate)
        {
            int index = -1;
            for (int i = 0; i < list.Count; i++)
            {
                if (predicate(list[i]))
                {
                    index = i;
                }
            }

            return index;
        }

        public static T Median<T>(this IEnumerable<T> enumerable)
            where T : IComparable<T>
        {
            T[] sorted = enumerable.OrderBy(x => x).ToArray();
            return sorted[sorted.Length / 2];
        }
    }
}