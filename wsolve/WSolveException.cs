using System;

namespace WSolve
{
    public class WSolveException : Exception
    {
        public WSolveException(string message)
            : base(message)
        {
        }
    }
}