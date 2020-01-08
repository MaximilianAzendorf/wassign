using System;
using WSolve.ExtraConditions.Constraints;

// ReSharper disable PossibleNullReferenceException

namespace WSolve.ExtraConditions
{
    public class FieldStateless<TOwner, TFieldType> : StatelessAccessor<TOwner>
    {
        public FieldStateless(TOwner owner) : base(owner) { }

        public static Constraint operator ==(FieldStateless<TOwner, TFieldType> left, TFieldType right)
            => new SetValueConstraint<TOwner, TFieldType>(left.Owner, right);
        
        public static Constraint operator ==(TFieldType left, FieldStateless<TOwner, TFieldType> right)
            => new SetValueConstraint<TOwner, TFieldType>(right.Owner, left);
        
        public static Constraint operator !=(FieldStateless<TOwner, TFieldType> left, TFieldType right)
            => new ForbidValueConstraint<TOwner, TFieldType>(left.Owner, right);
        
        public static Constraint operator !=(TFieldType left, FieldStateless<TOwner, TFieldType> right)
            => new ForbidValueConstraint<TOwner, TFieldType>(right.Owner, left);

        public static Constraint operator ==(FieldStateless<TOwner, TFieldType> left, FieldStateless<TOwner, TFieldType> right)
            => new EqualsConstraint<TOwner, TFieldType>(left.Owner, right.Owner);
        
        public static Constraint operator !=(FieldStateless<TOwner, TFieldType> left, FieldStateless<TOwner, TFieldType> right)
            => new EqualsNotConstraint<TOwner, TFieldType>(left.Owner, right.Owner);
        
        public override bool Equals(object obj) => throw new InvalidStatelessOperationException();
        public override int GetHashCode() => throw new InvalidStatelessOperationException();
    }
}