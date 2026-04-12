use thiserror::Error;

/// Result type used throughout the crate.
pub type Result<T> = std::result::Result<T, InputError>;

#[derive(Debug, Error, Clone)]
/// Error type used for input parsing and execution failures.
pub enum InputError {
    #[error("{0}")]
    /// A plain user-facing error message.
    Message(String),
    #[error("solver worker panicked: {0}")]
    /// A worker thread panicked while solving.
    WorkerPanic(String),
    #[error("solution is incomplete: {0}")]
    /// A formatter or solver needed a missing solution part.
    IncompleteSolution(&'static str),
}
