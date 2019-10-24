using System;

namespace WSolve
{
    public static class Parameter
    {
        public static IParameter<T> Create<T>(Func<MultiLevelGaSystem, T> func)
        {
            return new FuncParameter<T>(func);
        }

        private class FuncParameter<T> : IParameter<T>
        {
            private readonly Func<MultiLevelGaSystem, T> _func;

            public FuncParameter(Func<MultiLevelGaSystem, T> func)
            {
                _func = func;
            }

            public T Evalutate(MultiLevelGaSystem algorithmState)
            {
                return _func(algorithmState);
            }
        }
    }
}