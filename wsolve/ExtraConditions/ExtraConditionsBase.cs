namespace WSolve.ExtraConditions {
    public abstract class ExtraConditionsBase 
    {
        protected Chromosome _chromosome;

        protected ExtraConditionsBase(Chromosome chromosome)
        {
            _chromosome = chromosome;
        }

        public bool DirectResult { get; private set; } = true;

        protected void AddCondition(bool condition)
        {
            DirectResult &= condition;
        }
    }
}