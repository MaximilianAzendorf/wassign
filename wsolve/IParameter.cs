namespace wsolve
{
    public interface IParameter<out T>
    {
        T Evalutate(IGeneticAlgorithm algorithmState);
    }
}