use serde::{Deserialize, Serialize};

use crate::{Score, ThreadedSolverProgress};

/// Machine-readable progress stream event emitted by the CLI benchmark mode.
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum ProgressStreamEvent {
    /// Snapshot of the current solver progress.
    Progress {
        /// Latest solver progress snapshot.
        progress: ThreadedSolverProgress,
    },
    /// Final solve outcome for one invocation.
    Finished {
        /// Best score found, or `None` when no solution was found.
        score: Option<Score>,
    },
}
