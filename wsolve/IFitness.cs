using System;

namespace wsolve
{
    public interface IFitness
    {
        (float major, float minor) Evaluate(Chromosome chromosome);
    }
}