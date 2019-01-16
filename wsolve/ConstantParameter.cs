namespace wsolve
{
    public class ConstantParameter<T> : IParameter<T>
    {
        public T Value { get; }

        public ConstantParameter(T value)
        {
            Value = value;
        }

        public T Evalutate(IGeneticAlgorithm algorithmState) => Value;
        
        public static implicit operator ConstantParameter<T>(T x) => new ConstantParameter<T>(x);
    }
}