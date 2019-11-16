namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public class EqualsNotConstraint<TOwner, TFieldType> : Constraint
    {
        public TOwner Left { get; }
        public TOwner Right { get; }
        
        public EqualsNotConstraint(TOwner left, TOwner right)
        {
            Left = left;
            Right = right;
        }
    }
}