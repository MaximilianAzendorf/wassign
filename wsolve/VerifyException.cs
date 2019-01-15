using System;

namespace wsolve
{
    public class VerifyException : Exception
    {
        public VerifyException(string msg) : base(msg) {}
    }
}