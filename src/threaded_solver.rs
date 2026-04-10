use std::any::Any;
use std::sync::mpsc::{self, Receiver};
use std::thread::JoinHandle;
use std::time::{Duration, SystemTime};

use crate::cancellation::CancellationToken;
use crate::shotgun_solver::{ShotgunSolver, ShotgunSolverProgress, WorkerProgressEvent};
use crate::util::{time_now, time_never};
use crate::{InputData, InputError, Options, PreparedProblem, Result, Rng, Score, Scoring, Solution};

#[derive(Debug, Clone)]
pub struct ThreadedSolverProgress {
    pub(crate) sched_depth: f32,
    pub(crate) max_sched_depth: f32,
    pub(crate) iterations: usize,
    pub(crate) assignments: usize,
    pub(crate) lp: usize,
    pub(crate) best_solution: Solution,
    pub(crate) best_score: Score,
    pub(crate) milliseconds_remaining: i64,
}

/// Idle threaded solver state before a solve session is started.
#[derive(Debug)]
pub struct ThreadedSolver {
    problem: PreparedProblem,
    options: Options,
}

/// Running threaded solver state that owns the active solve session.
#[derive(Debug)]
pub struct ThreadedSolverRunning {
    options: Options,
    max_sched_depth: f32,
    thread_progress: Vec<ShotgunSolverProgress>,
    thread_start_times: Vec<SystemTime>,
    handle: JoinHandle<ThreadedSolverResult>,
    progress_rx: Receiver<WorkerProgressEvent>,
    cancellation: CancellationToken,
}

/// Final threaded solver result returned after the solve session completes.
#[derive(Debug)]
pub struct ThreadedSolverResult {
    /// Parsed input data needed for formatting and external inspection.
    pub input_data: InputData,
    /// Scoring configuration needed to evaluate the returned solution.
    pub scoring: Scoring,
    /// Best solution found during the solve session.
    pub solution: Solution,
}

impl ThreadedSolverResult {
    /// Returns the final solution.
    #[must_use]
    pub fn solution(&self) -> &Solution {
        &self.solution
    }

    /// Consumes the result and returns the final solution.
    #[must_use]
    pub fn into_solution(self) -> Solution {
        self.solution
    }
}

impl ThreadedSolver {
    /// Creates a threaded solver for the prepared problem.
    #[must_use]
    pub fn new(problem: PreparedProblem, options: Options) -> Self {
        Self { problem, options }
    }

    /// Starts the background solve session and returns the running solver state.
    pub fn start(self) -> Result<ThreadedSolverRunning> {
        let ThreadedSolver { problem, options } = self;
        let num_threads = if problem.input_data.slots.len() == 1 {
            1
        } else {
            usize::try_from(options.thread_count.max(1)).expect("thread count must be positive")
        };

        let max_sched_depth =
            u16::try_from(problem.input_data.choices.len()).map_or(f32::INFINITY, f32::from);
        let thread_progress = vec![empty_progress(); num_threads];
        let thread_start_times = vec![time_now(); num_threads];
        let thread_seeds = (0..num_threads).map(|tid| Rng::derived_seed(tid as u64)).collect::<Vec<_>>();
        let cancellation = CancellationToken::new();
        let cancellation_for_workers = cancellation.clone();
        let (progress_tx, progress_rx) = mpsc::channel();

        let options_for_thread = options.clone();
        let handle = std::thread::spawn(move || {
            let solution = std::thread::scope(|scope| {
                let problem = &problem;
                let options = &options_for_thread;
                let mut worker_handles = Vec::with_capacity(num_threads);
                for (tid, seed) in thread_seeds.into_iter().enumerate() {
                    let progress_tx = progress_tx.clone();
                    let cancellation = cancellation_for_workers.clone();
                    worker_handles.push(scope.spawn(move || {
                        let mut solver =
                            ShotgunSolver::new_with_seed(problem, options, progress_tx, tid, seed);
                        let start_time = time_now();
                        let timeout =
                            Duration::from_secs(u64::try_from(options.timeout_seconds).unwrap_or_default());
                        let deadline = start_time + timeout;

                        while !cancellation.is_cancelled() && time_now() <= deadline {
                            let iterations_done = solver.iterate(1, Some(deadline));
                            if problem.input_data.slots.len() == 1 || iterations_done < 1 {
                                break;
                            }
                        }

                        solver.progress().clone()
                    }));
                }

                let mut best_solution = Solution::Invalid;
                let mut best_score = Score {
                    major: f32::INFINITY,
                    minor: f32::INFINITY,
                };
                for handle in worker_handles {
                    let progress = handle.join().expect("worker should not panic");
                    if progress.best_score < best_score {
                        best_score = progress.best_score;
                        best_solution = progress.best_solution;
                    }
                }
                best_solution
            });

            ThreadedSolverResult {
                input_data: problem.input_data,
                scoring: problem.scoring,
                solution,
            }
        });

        Ok(ThreadedSolverRunning {
            options,
            max_sched_depth,
            thread_progress,
            thread_start_times,
            handle,
            progress_rx,
            cancellation,
        })
    }
}

impl ThreadedSolverRunning {
    pub(crate) fn is_running(&self) -> bool {
        !self.handle.is_finished()
    }

    pub(crate) fn timeout_seconds(&self) -> i32 {
        self.options.timeout_seconds
    }

    /// Requests cancellation of the running solve session.
    ///
    /// This only signals cancellation; it does not wait for the background
    /// solver thread to stop. Call [`Self::wait`] to join the thread and
    /// collect the final result.
    pub fn cancel(&self) {
        self.cancellation.cancel();
    }

    pub(crate) fn progress(&mut self) -> ThreadedSolverProgress {
        let mut progress = ThreadedSolverProgress {
            sched_depth: 0.0,
            max_sched_depth: self.max_sched_depth,
            iterations: 0,
            assignments: 0,
            lp: 0,
            best_solution: Solution::Invalid,
            best_score: Score {
                major: f32::INFINITY,
                minor: f32::INFINITY,
            },
            milliseconds_remaining: 0,
        };

        for event in self.progress_rx.try_iter() {
            if let Some(worker_progress) = self.thread_progress.get_mut(event.worker_index) {
                if let Some(sched_depth) = event.sched_depth {
                    worker_progress.sched_depth = sched_depth;
                }
                if let Some(iterations) = event.iterations {
                    worker_progress.iterations = iterations;
                }
                if let Some(assignments) = event.assignments {
                    worker_progress.assignments = assignments;
                }
                if let Some(lp) = event.lp {
                    worker_progress.lp = lp;
                }
                if let Some(best_score) = event.best_score {
                    worker_progress.best_score = best_score;
                }
                if let Some(best_solution) = event.best_solution {
                    worker_progress.best_solution = best_solution;
                }
            }
        }

        let thread_count = self.thread_progress.len().max(1);
        for (index, thread_progress) in self.thread_progress.iter().enumerate() {
            let elapsed = time_now()
                .duration_since(
                    self.thread_start_times
                        .get(index)
                        .copied()
                        .unwrap_or_else(time_never),
                )
                .unwrap_or_default();
            let timeout =
                Duration::from_secs(u64::try_from(self.options.timeout_seconds).unwrap_or_default());
            let remaining = timeout.saturating_sub(elapsed);
            progress.milliseconds_remaining = progress
                .milliseconds_remaining
                .max(i64::try_from(remaining.as_millis()).unwrap_or(i64::MAX));

            if thread_progress.best_score < progress.best_score {
                progress.best_score = thread_progress.best_score;
                progress.best_solution = thread_progress.best_solution.clone();
            }
            progress.sched_depth += thread_progress.sched_depth;
            progress.iterations += thread_progress.iterations;
            progress.assignments += thread_progress.assignments;
            progress.lp += thread_progress.lp;
        }

        progress.sched_depth /= u16::try_from(thread_count).map_or(f32::INFINITY, f32::from);
        progress
    }

    /// Waits for the current solve session to finish and returns the final result.
    pub fn wait(self) -> Result<ThreadedSolverResult> {
        match self.handle.join() {
            Ok(result) => Ok(result),
            Err(payload) => Err(InputError::WorkerPanic(panic_message(&payload))),
        }
    }
}

fn empty_progress() -> ShotgunSolverProgress {
    ShotgunSolverProgress {
        sched_depth: 0.0,
        iterations: 0,
        assignments: 0,
        lp: 0,
        best_solution: Solution::Invalid,
        best_score: Score {
            major: f32::INFINITY,
            minor: f32::INFINITY,
        },
    }
}

fn panic_message(payload: &Box<dyn Any + Send + 'static>) -> String {
    if let Some(message) = payload.downcast_ref::<String>() {
        return message.clone();
    }

    if let Some(message) = payload.downcast_ref::<&str>() {
        return (*message).to_owned();
    }

    "unknown panic payload".to_owned()
}
