use std::sync::Arc;

use crate::{Assignment, Scheduling};

/// Combined scheduling and assignment result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Solution {
    pub(crate) scheduling: Option<Arc<Scheduling>>,
    pub(crate) assignment: Option<Arc<Assignment>>,
}

impl Solution {
    /// Creates a solution from its scheduling and assignment parts.
    #[must_use]
    pub fn new(scheduling: Option<Arc<Scheduling>>, assignment: Option<Arc<Assignment>>) -> Self {
        Self { scheduling, assignment }
    }

    /// Returns `true` when the solution is incomplete.
    #[must_use]
    pub fn is_invalid(&self) -> bool {
        self.scheduling.is_none() || self.assignment.is_none()
    }

    pub(crate) fn invalid() -> Self {
        Self::new(None, None)
    }
}
