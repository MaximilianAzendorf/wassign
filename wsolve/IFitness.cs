using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    public interface IFitness<T> 
        where T : IComparable<T>
    {
        T Evaluate(Chromosome chromosome);
    }
}
