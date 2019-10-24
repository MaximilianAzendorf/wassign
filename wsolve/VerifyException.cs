using System;

namespace WSolve
{
    public class VerifyException : Exception
    {
        public VerifyException(string msg)
            : base(msg)
        {
        }
    }
}