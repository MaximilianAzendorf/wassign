namespace WSolve
{
    public interface IParameter<out T>
    {
        T Evalutate(MultiLevelGaSystem algorithmState);
    }
}