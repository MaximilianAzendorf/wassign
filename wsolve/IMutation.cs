using System;
using System.Collections.Generic;
using System.Text;

namespace wsolve
{
    public interface IMutation
    {
        void Mutate(Chromosome chromosome);
    }
}
