using System;

namespace WSolve
{
    public class VerifyException : Exception
    {
        public VerifyException(string message)
            : base(message) { }
    }
}