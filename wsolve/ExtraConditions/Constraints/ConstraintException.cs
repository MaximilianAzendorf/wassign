using System;

namespace WSolve.ExtraConditions.Constraints {
    public class ConstraintException : Exception
    {
        public ConstraintException(string message)
            : base(message)
        {
        }
    }
}