using System;

namespace WSolve
{
    public static class Parameter
    {
        private class ConstantParameter<T> : IParameter<T>
        {
            private readonly T _value;

            public ConstantParameter(T value)
            {
                _value = value;
            }

            public T Evalutate(MultiLevelGaSystem algorithmState) => _value;
        }

        private class FuncParameter<T> : IParameter<T>
        {
            private readonly Func<MultiLevelGaSystem, T> _func;

            public FuncParameter(Func<MultiLevelGaSystem, T> func)
            {
                _func = func;
            }

            public T Evalutate(MultiLevelGaSystem algorithmState) => _func(algorithmState);
        }
        
        public static IParameter<T> Create<T>(T value) => new ConstantParameter<T>(value);
        
        public static IParameter<T> Create<T>(Func<T> func) => new FuncParameter<T>(x => func());
        
        public static IParameter<T> Create<T>(Func<MultiLevelGaSystem, T> func) => new FuncParameter<T>(func);
    }
}