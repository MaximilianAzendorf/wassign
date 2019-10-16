using System;
using System.Collections.Generic;
using System.Text;

namespace WSolve
{
    public interface IMutation
    {
        void Mutate(Chromosome chromosome);
    }
}
