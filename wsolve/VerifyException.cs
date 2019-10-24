namespace WSolve
{
    using System;

    public class VerifyException : Exception
    {
        public VerifyException(string msg)
            : base(msg)
        {
        }
    }
}