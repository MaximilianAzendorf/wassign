use std::any::Any;
use std::sync::mpsc::{self, Receiver};
use std::thread::JoinHandle;
use std::time::{Duration, SystemTime};

use serde::{Deserialize, Serialize};

use crate::cancellation::CancellationToken;
use crate::shotgun_solver::{ShotgunSolver, ShotgunSolverProgress, WorkerProgressEvent};
use crate::util::{time_never, time_now};
use crate::{
    InputData, InputError, Options, PreparedProblem, Result, Rng, Score, Scoring, Solution,
};

/// Aggregated progress snapshot for a running threaded solve session.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ThreadedSolverProgress {
    /// Current average scheduling depth across worker threads.
    pub sched_depth: f32,
    /// Maximum scheduling depth for the problem.
    pub max_sched_depth: f32,
    /// Number of outer shotgun iterations explored.
    pub iterations: usize,
    /// Number of assignment attempts executed.
    pub assignments: usize,
    /// Number of linear-programming solves executed.
    pub lp: usize,
    #[serde(skip, default)]
    /// Best solution found so far.
    pub best_solution: Solution,
    /// Best score found so far.
    pub best_score: Score,
    /// Milliseconds remaining until the configured timeout expires.
    pub milliseconds_remaining: i64,
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
    ///
    /// # Errors
    ///
    /// This function currently does not return an error during startup, but it
    /// uses `Result` to align with the rest of the solver API.
    ///
    /// # Panics
    ///
    /// Panics if `thread_count` cannot be converted to `usize`.
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
        let thread_seeds = (0..num_threads)
            .map(|tid| Rng::derived_seed(tid as u64))
            .collect::<Vec<_>>();
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
                        let timeout = Duration::from_secs(
                            u64::try_from(options.timeout_seconds).unwrap_or_default(),
                        );
                        let deadline = start_time + timeout;

                        while !cancellation.is_cancelled() && time_now() <= deadline {
                            let iterations_done = solver.iterate(1, Some(deadline));
                            if options.any && solver.progress().best_score.is_finite() {
                                cancellation.cancel();
                                break;
                            }
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
    /// Returns whether the solve session is still running.
    #[must_use]
    pub fn is_running(&self) -> bool {
        !self.handle.is_finished()
    }

    /// Returns the configured solve timeout in seconds.
    #[must_use]
    pub fn timeout_seconds(&self) -> i32 {
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

    /// Polls and returns the latest aggregated progress snapshot.
    #[must_use]
    pub fn progress(&mut self) -> ThreadedSolverProgress {
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
                if let (Some(best_score), Some(best_solution)) =
                    (event.best_score, event.best_solution)
                    && best_score < worker_progress.best_score
                {
                    worker_progress.best_score = best_score;
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
            let timeout = Duration::from_secs(
                u64::try_from(self.options.timeout_seconds).unwrap_or_default(),
            );
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
    ///
    /// # Errors
    ///
    /// Returns [`InputError::WorkerPanic`] if the background solver thread
    /// terminated with a panic.
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

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;
    use std::sync::mpsc;
    use std::thread;

    use super::*;

    fn empty_input_data() -> InputData {
        InputData {
            choices: Vec::new(),
            choosers: Vec::new(),
            slots: Vec::new(),
            scheduling_constraints: Vec::new(),
            assignment_constraints: Vec::new(),
            dependent_choice_groups: Vec::new(),
            preference_levels: Vec::new(),
            max_preference: 0,
            choice_constraint_map: BTreeMap::new(),
            chooser_constraint_map: BTreeMap::new(),
        }
    }

    fn dummy_running_solver(progress_rx: Receiver<WorkerProgressEvent>) -> ThreadedSolverRunning {
        let options = Options::default();
        ThreadedSolverRunning {
            options: options.clone(),
            max_sched_depth: 1.0,
            thread_progress: vec![empty_progress()],
            thread_start_times: vec![time_now()],
            handle: thread::spawn(|| ThreadedSolverResult {
                input_data: empty_input_data(),
                scoring: Scoring::new(&empty_input_data(), &Options::default()),
                solution: Solution::Invalid,
            }),
            progress_rx,
            cancellation: CancellationToken::new(),
        }
    }

    #[test]
    fn progress_keeps_worker_best_score_monotonic() {
        let (tx, rx) = mpsc::channel();
        let mut solver = dummy_running_solver(rx);
        let improved_solution = Solution::Invalid;

        tx.send(WorkerProgressEvent {
            worker_index: 0,
            sched_depth: None,
            iterations: None,
            assignments: None,
            lp: None,
            best_solution: Some(improved_solution.clone()),
            best_score: Some(Score {
                major: 45.0,
                minor: 1.5,
            }),
        })
        .expect("event should send");
        tx.send(WorkerProgressEvent {
            worker_index: 0,
            sched_depth: None,
            iterations: None,
            assignments: None,
            lp: None,
            best_solution: Some(Solution::Invalid),
            best_score: Some(Score {
                major: 45.0,
                minor: 1.8,
            }),
        })
        .expect("event should send");

        let progress = solver.progress();

        assert_eq!(
            progress.best_score,
            Score {
                major: 45.0,
                minor: 1.5
            }
        );
        assert_eq!(progress.best_solution, improved_solution);
    }
}
