//! `wassign`, a solver for constrained scheduling and assignment problems.
//!
//! The crate preserves the original program's two-stage architecture:
//! choices are first scheduled into slots and are then assigned to choosers while
//! respecting additional assignment constraints and optimizing preference quality.

mod assignment;
mod assignment_solver;
mod choice_data;
mod chooser_data;
mod constraint;
mod constraints;
mod critical_set;
mod critical_set_analysis;
mod error;
mod hill_climbing_solver;
mod input;
mod input_data;
mod mip_flow;
mod mip_flow_static_data;
mod options;
mod output_formatter;
mod rng;
mod scheduling;
mod scheduling_solver;
mod score;
mod scoring;
mod shotgun_solver;
mod shotgun_solver_threaded;
mod slot_data;
mod solution;
mod status;
mod union_find;
mod util;
mod version;

pub use assignment::Assignment;
pub use assignment_solver::AssignmentSolver;
pub use choice_data::ChoiceData;
pub use chooser_data::ChooserData;
pub use critical_set_analysis::CriticalSetAnalysis;
pub use error::{InputError, Result};
pub use input::input_reader::InputReader;
pub use input_data::InputData;
pub use mip_flow_static_data::MipFlowStaticData;
pub use options::{Options, OptionsParseStatus};
pub use output_formatter::OutputFormatter;
pub use rng::Rng;
pub use scheduling::Scheduling;
pub use scheduling_solver::SchedulingSolver;
pub use scoring::Scoring;
pub use solution::Solution;
pub use slot_data::SlotData;
pub use shotgun_solver_threaded::ShotgunSolverThreaded;
pub use status::Status;
pub use version::WASSIGN_VERSION;

pub(crate) use constraint::{Constraint, ConstraintType, SlotSizeLimitOp};
pub(crate) use constraints::Constraints;
pub(crate) use critical_set::CriticalSet;
pub(crate) use hill_climbing_solver::HillClimbingSolver;
pub(crate) use mip_flow::MipFlow;
pub(crate) use score::Score;
pub(crate) use shotgun_solver::ShotgunSolver;
pub(crate) use union_find::UnionFind;
