use crate::{CriticalSetAnalysis, InputData, MipFlowStaticData, Options, Scoring};

/// Immutable problem data prepared once per run and borrowed by solver workers.
#[derive(Debug, Clone)]
pub struct PreparedProblem {
    /// Parsed input data for the prepared problem.
    pub input_data: InputData,
    /// Critical-set analysis derived from the input data.
    pub critical_set_analysis: CriticalSetAnalysis,
    /// Static flow graph data reused across assignment solves.
    pub static_flow_data: MipFlowStaticData,
    /// Scoring configuration for evaluating candidate solutions.
    pub scoring: Scoring,
}

impl PreparedProblem {
    /// Builds immutable derived solver state for a parsed input and options.
    #[must_use]
    pub fn new(input_data: InputData, options: &Options) -> Self {
        let do_cs_analysis = !options.no_critical_sets && !options.greedy && input_data.slots.len() > 1;
        let do_cs_simplification = do_cs_analysis && !options.no_critical_set_simplification;
        let critical_set_analysis =
            CriticalSetAnalysis::new(&input_data, do_cs_analysis, do_cs_simplification);
        let static_flow_data = MipFlowStaticData::new(&input_data);
        let scoring = Scoring::new(&input_data, options);

        Self {
            input_data,
            critical_set_analysis,
            static_flow_data,
            scoring,
        }
    }
}
