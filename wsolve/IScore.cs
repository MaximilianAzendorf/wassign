namespace WSolve
{
    public interface IScore
    {
        (float major, float minor) Evaluate(Candidate candidate);

        bool IsFeasible(Candidate candidate);
    }
}