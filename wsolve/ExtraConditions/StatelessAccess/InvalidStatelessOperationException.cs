using System;
using System.Runtime.CompilerServices;

namespace WSolve.ExtraConditions.StatelessAccess
{
    public class InvalidStatelessOperationException : InvalidOperationException
    {
        public InvalidStatelessOperationException([CallerMemberName]string memberName = null)
            : base($"\'{memberName}\' is not a valid operation while constructing static constraints.")
        {
        }
    }
}