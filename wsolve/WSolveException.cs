namespace WSolve
{
    using System;

    public class WSolveException : Exception
    {
        public WSolveException(string message) 
            : base(message)
        {
        }
    }
}