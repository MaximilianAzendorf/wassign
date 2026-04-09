#![expect(
    clippy::cast_precision_loss,
    clippy::cast_sign_loss,
    reason = "threaded progress reporting converts bounded counters and durations for display"
)]

use std::sync::atomic::{AtomicBool, Ordering};
use std::any::Any;
use std::sync::{Arc, Mutex};
use std::thread::JoinHandle;
use std::time::{Duration, SystemTime};

use crate::{
    CriticalSetAnalysis, InputData, InputError, MipFlowStaticData, Options, Result, Rng, Score, Scoring,
    ShotgunSolver, Solution,
};
use crate::shotgun_solver::ShotgunSolverProgress;
use crate::Status;
use crate::util::{time_never, time_now};

#[derive(Debug, Clone)]
pub(crate) struct ShotgunSolverThreadedProgress {
    pub(crate) sched_depth: f32,
    pub(crate) max_sched_depth: f32,
    pub(crate) iterations: usize,
    pub(crate) assignments: usize,
    pub(crate) lp: usize,
    pub(crate) best_solution: Solution,
    pub(crate) best_score: Score,
    pub(crate) milliseconds_remaining: i64,
}

/// Multi-threaded top-level solver that combines scheduling and assignment search.
#[derive(Debug)]
pub struct ShotgunSolverThreaded {
    input_data: Arc<InputData>,
    options: Arc<Options>,
    cs_analysis: Arc<CriticalSetAnalysis>,
    static_data: Arc<MipFlowStaticData>,
    scoring: Arc<Scoring>,
    thread_solvers: Vec<ShotgunSolver>,
    thread_progress: Vec<Arc<Mutex<ShotgunSolverProgress>>>,
    thread_seeds: Vec<u64>,
    thread_start_times: Vec<SystemTime>,
    handles: Vec<Option<JoinHandle<ShotgunSolver>>>,
    cancellation: Arc<AtomicBool>,
}

impl ShotgunSolverThreaded {
    /// Creates a new threaded solver.
    #[must_use]
    pub fn new(
        input_data: Arc<InputData>,
        cs_analysis: Arc<CriticalSetAnalysis>,
        static_data: Arc<MipFlowStaticData>,
        scoring: Arc<Scoring>,
        options: Arc<Options>,
    ) -> Self {
        Self {
            input_data,
            options,
            cs_analysis,
            static_data,
            scoring,
            thread_solvers: Vec::new(),
            thread_progress: Vec::new(),
            thread_seeds: Vec::new(),
            thread_start_times: Vec::new(),
            handles: Vec::new(),
            cancellation: Arc::new(AtomicBool::new(false)),
        }
    }

    pub(crate) fn is_running(&self) -> bool {
        self.handles.iter().any(|handle| handle.as_ref().is_some_and(|join| !join.is_finished()))
    }

    pub(crate) fn timeout_seconds(&self) -> i32 {
        self.options.timeout_seconds
    }

    /// Starts the worker threads.
    ///
    /// # Errors
    ///
    /// Returns an error if the solver is already running or if cleanup after a
    /// previous run failed.
    ///
    /// # Panics
    ///
    /// Panics if the configured thread count does not fit in `usize`.
    pub fn start(&mut self) -> Result<()> {
        if self.is_running() {
            return Err(InputError::Message("solver is already running".to_owned()));
        }

        self.cancel()?;
        Status::debug("Preparing threaded solver state.");

        let num_threads = if self.input_data.slot_count() == 1 {
            1
        } else {
            usize::try_from(self.options.thread_count.max(1)).expect("thread count must be positive")
        };
        Status::debug(&format!(
            "Starting solving phase with {num_threads} worker thread(s) and timeout {}s.",
            self.options.timeout_seconds
        ));

        self.thread_solvers.clear();
        self.thread_progress.clear();
        self.thread_seeds.clear();
        self.thread_start_times = vec![time_never(); num_threads];
        self.handles = Vec::with_capacity(num_threads);
        self.cancellation = Arc::new(AtomicBool::new(false));

        for tid in 0..num_threads {
            let worker_seed = Rng::derived_seed(tid as u64);
            let input_data = self.input_data.clone();
            let options = self.options.clone();
            let cs_analysis = self.cs_analysis.clone();
            let static_data = self.static_data.clone();
            let scoring = self.scoring.clone();
            let cancellation = self.cancellation.clone();
            let progress = Arc::new(Mutex::new(empty_progress()));
            self.thread_progress.push(progress.clone());
            self.thread_seeds.push(worker_seed);
            self.thread_start_times[tid] = time_now();

            let handle = std::thread::spawn(move || {
                Status::trace(&format!("Worker {tid} started."));
                let mut solver =
                    ShotgunSolver::new_with_seed(
                        input_data.clone(),
                        cs_analysis,
                        static_data,
                        scoring,
                        options.clone(),
                        Some(progress.clone()),
                        worker_seed,
                    );
                let start_time = time_now();
                let timeout = Duration::from_secs(options.timeout_seconds as u64);
                let deadline = start_time + timeout;

                while !cancellation.load(Ordering::Relaxed) && time_now() <= deadline {
                    let iterations_done = solver.iterate(1, Some(deadline));
                    *progress.lock().expect("progress mutex poisoned") = solver.progress().clone();
                    if input_data.slot_count() == 1 || iterations_done < 1 {
                        break;
                    }
                }

                *progress.lock().expect("progress mutex poisoned") = solver.progress().clone();
                Status::trace(&format!("Worker {tid} finished."));

                solver
            });
            self.handles.push(Some(handle));
        }

        Ok(())
    }

    pub(crate) fn cancel(&mut self) -> Result<()> {
        self.cancellation.store(true, Ordering::Relaxed);
        self.join_finished(true)
    }

    /// Waits for the workers to finish and returns the best solution found.
    ///
    /// # Errors
    ///
    /// Returns an error if a worker thread panicked or if internal cleanup
    /// failed.
    pub fn wait_for_result(&mut self) -> Result<Solution> {
        if !self.is_running() {
            self.cancellation.store(true, Ordering::Relaxed);
        }

        self.join_finished(true)?;
        Ok(self.current_solution())
    }

    pub(crate) fn current_solution(&self) -> Solution {
        self.progress().best_solution
    }

    pub(crate) fn progress(&self) -> ShotgunSolverThreadedProgress {
        let mut progress = ShotgunSolverThreadedProgress {
            sched_depth: 0.0,
            max_sched_depth: self.input_data.choice_count() as f32,
            iterations: 0,
            assignments: 0,
            lp: 0,
            best_solution: Solution::invalid(),
            best_score: Score {
                major: f32::INFINITY,
                minor: f32::INFINITY,
            },
            milliseconds_remaining: 0,
        };

        let thread_count = self.handles.len().max(1);
        for (index, maybe_handle) in self.handles.iter().enumerate() {
            let elapsed = time_now()
                .duration_since(self.thread_start_times.get(index).copied().unwrap_or_else(time_never))
                .unwrap_or_default();
            let timeout = Duration::from_secs(self.options.timeout_seconds as u64);
            let remaining = timeout.saturating_sub(elapsed);
            progress.milliseconds_remaining =
                progress.milliseconds_remaining.max(i64::try_from(remaining.as_millis()).unwrap_or(i64::MAX));

            let _ = maybe_handle;
            let Some(thread_progress) = self.thread_progress.get(index) else {
                continue;
            };
            let thread_progress = thread_progress.lock().expect("progress mutex poisoned").clone();
            if thread_progress.best_score < progress.best_score {
                progress.best_score = thread_progress.best_score;
                progress.best_solution = thread_progress.best_solution.clone();
            }
            progress.sched_depth += thread_progress.sched_depth;
            progress.iterations += thread_progress.iterations;
            progress.assignments += thread_progress.assignments;
            progress.lp += thread_progress.lp;
        }

        progress.sched_depth /= thread_count as f32;
        progress
    }

    fn join_finished(&mut self, block: bool) -> Result<()> {
        if self.handles.is_empty() {
            return Ok(());
        }

        if self.thread_solvers.len() < self.handles.len() {
            let start_len = self.thread_solvers.len();
            for index in start_len..self.handles.len() {
                self.thread_solvers.push(ShotgunSolver::new_with_seed(
                    self.input_data.clone(),
                    self.cs_analysis.clone(),
                    self.static_data.clone(),
                    self.scoring.clone(),
                    self.options.clone(),
                    None,
                    self.thread_seeds[index],
                ));
            }
        }

        for index in 0..self.handles.len() {
            let should_join = self.handles[index]
                .as_ref()
                .is_some_and(|handle| block || handle.is_finished());
            if !should_join {
                continue;
            }

            let Some(handle) = self.handles[index].take() else {
                return Err(InputError::Message("solver worker handle was unexpectedly missing".to_owned()));
            };
            match handle.join() {
                Ok(solver) => {
                    self.thread_solvers[index] = solver;
                    if let Some(progress) = self.thread_progress.get(index) {
                        *progress.lock().expect("progress mutex poisoned") =
                            self.thread_solvers[index].progress().clone();
                    }
                }
                Err(payload) => {
                    return Err(InputError::WorkerPanic(panic_message(payload)));
                }
            }
        }

        Ok(())
    }
}

fn empty_progress() -> ShotgunSolverProgress {
    ShotgunSolverProgress {
        sched_depth: 0.0,
        iterations: 0,
        assignments: 0,
        lp: 0,
        best_solution: Solution::invalid(),
        best_score: Score {
            major: f32::INFINITY,
            minor: f32::INFINITY,
        },
    }
}

#[expect(clippy::needless_pass_by_value, reason = "panic payloads are owned by JoinHandle::join")]
fn panic_message(payload: Box<dyn Any + Send + 'static>) -> String {
    if let Some(message) = payload.downcast_ref::<String>() {
        return message.clone();
    }

    if let Some(message) = payload.downcast_ref::<&str>() {
        return (*message).to_owned();
    }

    "unknown panic payload".to_owned()
}
