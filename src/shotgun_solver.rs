#![expect(
    clippy::cast_precision_loss,
    reason = "progress snapshots are display-oriented"
)]

use std::sync::{Arc, Mutex};
use std::time::SystemTime;

use crate::{
    CriticalSetAnalysis, HillClimbingSolver, InputData, MipFlowStaticData, Options, Rng, Score, Scoring,
    SchedulingSolver, Solution, Status,
};

#[derive(Debug, Clone)]
pub(crate) struct ShotgunSolverProgress {
    pub(crate) sched_depth: f32,
    pub(crate) iterations: usize,
    pub(crate) assignments: usize,
    pub(crate) lp: usize,
    pub(crate) best_solution: Solution,
    pub(crate) best_score: Score,
}

#[derive(Debug)]
pub(crate) struct ShotgunSolver {
    scoring: Arc<Scoring>,
    progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
    pub(crate) hill_climbing_solver: HillClimbingSolver,
    pub(crate) scheduling_solver: SchedulingSolver,
    pub(crate) progress: ShotgunSolverProgress,
}

impl ShotgunSolver {
    pub(crate) fn new_with_seed(
        input_data: Arc<InputData>,
        cs_analysis: Arc<CriticalSetAnalysis>,
        static_data: Arc<MipFlowStaticData>,
        scoring: Arc<Scoring>,
        options: Arc<Options>,
        progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
        seed: u64,
    ) -> Self {
        let scoring_for_children = scoring.clone();
        let progress_sink_for_children = progress_sink.clone();
        let scheduling_rng = Rng::from_seed(seed);
        let hill_rng = Rng::from_seed(seed.wrapping_add(1));
        let mut scheduling_solver =
            SchedulingSolver::new_with_rng(input_data.clone(), cs_analysis.clone(), options.clone(), scheduling_rng);
        if let Some(progress_sink) = &progress_sink_for_children {
            scheduling_solver.set_progress_sink(progress_sink.clone());
        }

        Self {
            scoring,
            progress_sink,
            hill_climbing_solver: HillClimbingSolver::new_with_rng(
                input_data,
                cs_analysis,
                static_data,
                scoring_for_children,
                options,
                progress_sink_for_children,
                hill_rng,
            ),
            scheduling_solver,
            progress: ShotgunSolverProgress {
                sched_depth: 0.0,
                iterations: 0,
                assignments: 0,
                lp: 0,
                best_solution: Solution::invalid(),
                best_score: Score {
                    major: f32::INFINITY,
                    minor: f32::INFINITY,
                },
            },
        }
    }

    pub(crate) fn progress(&mut self) -> &ShotgunSolverProgress {
        self.progress.sched_depth = self.scheduling_solver.max_depth_reached as f32;
        self.progress.assignments = self.hill_climbing_solver.assignment_count;
        self.progress.lp = self.hill_climbing_solver.assignment_solver.lp_count;
        self.publish_progress_snapshot();
        &self.progress
    }

    pub(crate) fn iterate(&mut self, number_of_iterations: usize, deadline: Option<SystemTime>) -> usize {
        let mut iteration = 0_usize;
        while iteration < number_of_iterations {
            if deadline.is_some_and(|deadline| SystemTime::now() >= deadline) {
                Status::debug("Stopping shotgun iteration loop because the worker deadline was reached.");
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
            let score = self.scoring.evaluate(&solution);
            if score < self.progress.best_score {
                Status::debug(&format!("Found improved solution with score {}.", score.to_str()));
                self.progress.best_solution = solution;
                self.progress.best_score = score;
                self.publish_progress_snapshot();
            }

            self.progress.iterations += 1;
            self.publish_progress_snapshot();
            iteration += 1;
        }

        iteration
    }

    fn publish_progress_snapshot(&self) {
        let Some(progress_sink) = &self.progress_sink else {
            return;
        };

        let mut progress = progress_sink.lock().expect("progress mutex poisoned");
        progress.sched_depth = self.scheduling_solver.current_depth as f32;
        progress.iterations = self.progress.iterations;
        progress.assignments = self.hill_climbing_solver.assignment_count;
        progress.lp = self.hill_climbing_solver.assignment_solver.lp_count;
        if self.progress.best_score < progress.best_score {
            progress.best_score = self.progress.best_score;
            progress.best_solution = self.progress.best_solution.clone();
        }
    }
}
