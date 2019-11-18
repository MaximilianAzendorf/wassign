namespace WSolve.ExtraConditions.StatelessAccess.Constraints
{
    public class EqualsConstraint<TOwner, TFieldType> : Constraint
    {
        public TOwner Left { get; }
        public TOwner Right { get; }
        
        public EqualsConstraint(TOwner left, TOwner right)
        {
            Left = left;
            Right = right;
        }

        protected override Constraint Negation => new EqualsNotConstraint<TOwner, TFieldType>(Left, Right);
    }
}