use std::sync::Arc;

use crate::{Assignment, Scheduling};

/// Combined scheduling and assignment result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Solution {
    /// No usable scheduling or assignment is available.
    Invalid,
    /// A feasible scheduling exists, but no assignment has been found for it.
    Scheduling(Arc<Scheduling>),
    /// Both scheduling and assignment are available.
    Complete {
        /// The scheduling stage result.
        scheduling: Arc<Scheduling>,
        /// The assignment stage result for the scheduling.
        assignment: Arc<Assignment>,
    },
}

impl Solution {
    /// Creates a solution from its scheduling and assignment parts.
    ///
    /// # Panics
    ///
    /// Panics if an assignment is provided without a scheduling.
    #[must_use]
    pub fn new(scheduling: Option<Arc<Scheduling>>, assignment: Option<Arc<Assignment>>) -> Self {
        match (scheduling, assignment) {
            (Some(scheduling), Some(assignment)) => Self::Complete {
                scheduling,
                assignment,
            },
            (Some(scheduling), None) => Self::Scheduling(scheduling),
            (None, None) => Self::Invalid,
            (None, Some(_)) => {
                panic!("a solution cannot contain an assignment without a scheduling")
            }
        }
    }

    /// Returns `true` when the solution does not contain both parts.
    #[must_use]
    pub fn is_invalid(&self) -> bool {
        !matches!(self, Self::Complete { .. })
    }

    #[must_use]
    pub(crate) fn scheduling(&self) -> Option<&Arc<Scheduling>> {
        match self {
            Self::Invalid => None,
            Self::Scheduling(scheduling) | Self::Complete { scheduling, .. } => Some(scheduling),
        }
    }

    #[must_use]
    pub(crate) fn assignment(&self) -> Option<&Arc<Assignment>> {
        match self {
            Self::Complete { assignment, .. } => Some(assignment),
            Self::Invalid | Self::Scheduling(_) => None,
        }
    }
}
