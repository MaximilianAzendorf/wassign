use std::sync::mpsc::Sender;
use std::time::SystemTime;

use crate::status;
use crate::{HillClimbingSolver, Options, PreparedProblem, Rng, SchedulingSolver, Score, Solution};

#[derive(Debug, Clone)]
pub(crate) struct ShotgunSolverProgress {
    pub(crate) sched_depth: f32,
    pub(crate) iterations: usize,
    pub(crate) assignments: usize,
    pub(crate) lp: usize,
    pub(crate) best_solution: Solution,
    pub(crate) best_score: Score,
}

#[derive(Debug, Clone)]
pub(crate) struct WorkerProgressEvent {
    pub(crate) worker_index: usize,
    pub(crate) sched_depth: Option<f32>,
    pub(crate) iterations: Option<usize>,
    pub(crate) assignments: Option<usize>,
    pub(crate) lp: Option<usize>,
    pub(crate) best_solution: Option<Solution>,
    pub(crate) best_score: Option<Score>,
}

#[derive(Debug, Clone)]
pub(crate) struct ProgressReporter {
    tx: Sender<WorkerProgressEvent>,
    worker_index: usize,
}

impl ProgressReporter {
    pub(crate) fn new(tx: Sender<WorkerProgressEvent>, worker_index: usize) -> Self {
        Self { tx, worker_index }
    }

    pub(crate) fn dummy() -> Self {
        let (tx, _rx) = std::sync::mpsc::channel();
        Self::new(tx, 0)
    }

    pub(crate) fn publish_depth(&self, sched_depth: f32) {
        self.publish(WorkerProgressEvent {
            worker_index: self.worker_index,
            sched_depth: Some(sched_depth),
            iterations: None,
            assignments: None,
            lp: None,
            best_solution: None,
            best_score: None,
        });
    }

    pub(crate) fn publish_iterations(&self, iterations: usize) {
        self.publish(WorkerProgressEvent {
            worker_index: self.worker_index,
            sched_depth: None,
            iterations: Some(iterations),
            assignments: None,
            lp: None,
            best_solution: None,
            best_score: None,
        });
    }

    pub(crate) fn publish_assignments(&self, assignments: usize) {
        self.publish(WorkerProgressEvent {
            worker_index: self.worker_index,
            sched_depth: None,
            iterations: None,
            assignments: Some(assignments),
            lp: None,
            best_solution: None,
            best_score: None,
        });
    }

    pub(crate) fn publish_lp(&self, lp: usize) {
        self.publish(WorkerProgressEvent {
            worker_index: self.worker_index,
            sched_depth: None,
            iterations: None,
            assignments: None,
            lp: Some(lp),
            best_solution: None,
            best_score: None,
        });
    }

    pub(crate) fn publish_best_solution(&self, best_solution: Solution, best_score: Score) {
        self.publish(WorkerProgressEvent {
            worker_index: self.worker_index,
            sched_depth: None,
            iterations: None,
            assignments: None,
            lp: None,
            best_solution: Some(best_solution),
            best_score: Some(best_score),
        });
    }

    fn publish(&self, event: WorkerProgressEvent) {
        let _ = self.tx.send(event);
    }
}

#[derive(Debug)]
pub(crate) struct ShotgunSolver<'a> {
    problem: &'a PreparedProblem,
    progress_reporter: ProgressReporter,
    pub(crate) hill_climbing_solver: HillClimbingSolver<'a>,
    pub(crate) scheduling_solver: SchedulingSolver<'a>,
    pub(crate) progress: ShotgunSolverProgress,
}

impl<'a> ShotgunSolver<'a> {
    pub(crate) fn new_with_seed(
        problem: &'a PreparedProblem,
        options: &'a Options,
        progress_tx: Sender<WorkerProgressEvent>,
        worker_index: usize,
        seed: u64,
    ) -> Self {
        let progress_reporter = ProgressReporter::new(progress_tx, worker_index);
        let scheduling_rng = Rng::from_seed(seed);
        let hill_rng = Rng::from_seed(seed.wrapping_add(1));
        Self {
            problem,
            scheduling_solver: SchedulingSolver::new_with_rng(
                problem,
                options,
                progress_reporter.clone(),
                scheduling_rng,
            ),
            hill_climbing_solver: HillClimbingSolver::new_with_rng(
                problem,
                options,
                progress_reporter.clone(),
                hill_rng,
            ),
            progress_reporter,
            progress: ShotgunSolverProgress {
                sched_depth: 0.0,
                iterations: 0,
                assignments: 0,
                lp: 0,
                best_solution: Solution::Invalid,
                best_score: Score {
                    major: f32::INFINITY,
                    minor: f32::INFINITY,
                },
            },
        }
    }

    pub(crate) fn progress(&mut self) -> &ShotgunSolverProgress {
        self.progress.sched_depth = u16::try_from(self.scheduling_solver.max_depth_reached)
            .map_or(f32::INFINITY, f32::from);
        self.progress.assignments = self.hill_climbing_solver.assignment_count;
        self.progress.lp = self.hill_climbing_solver.assignment_solver.lp_count;
        &self.progress
    }

    pub(crate) fn iterate(
        &mut self,
        number_of_iterations: usize,
        deadline: Option<SystemTime>,
    ) -> usize {
        let mut iteration = 0_usize;
        while iteration < number_of_iterations {
            if deadline.is_some_and(|deadline| SystemTime::now() >= deadline) {
                status::debug(
                    "Stopping shotgun iteration loop because the worker deadline was reached.",
                );
                break;
            }

            if !self.scheduling_solver.next_scheduling(deadline) {
                break;
            }

            let scheduling = self
                .scheduling_solver
                .scheduling()
                .expect("next_scheduling must populate the current scheduling");
            let solution = self.hill_climbing_solver.solve(&scheduling, deadline);
            let score = self
                .problem
                .scoring
                .evaluate(&self.problem.input_data, &solution);
            if score < self.progress.best_score {
                status::debug(&format!("Found improved solution with score {score}."));
                self.progress.best_solution = solution.clone();
                self.progress.best_score = score;
                self.progress_reporter
                    .publish_best_solution(solution, score);
            }

            self.progress.iterations += 1;
            self.progress_reporter
                .publish_iterations(self.progress.iterations);
            iteration += 1;
        }

        iteration
    }
}
