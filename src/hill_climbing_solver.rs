use std::sync::{Arc, Mutex};
use std::time::SystemTime;

use crate::shotgun_solver::ShotgunSolverProgress;
use crate::status;
use crate::{
    AssignmentSolver, CriticalSetAnalysis, InputData, MipFlowStaticData, Options, Rng, Scheduling,
    Scoring, Solution,
};

#[derive(Debug)]
pub(crate) struct HillClimbingSolver {
    input_data: Arc<InputData>,
    scoring: Arc<Scoring>,
    options: Arc<Options>,
    rng: Rng,
    progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
    pub(crate) assignment_count: usize,
    pub(crate) assignment_solver: AssignmentSolver,
}

impl HillClimbingSolver {
    pub(crate) fn new_with_rng(
        input_data: Arc<InputData>,
        cs_analysis: &Arc<CriticalSetAnalysis>,
        static_data: &Arc<MipFlowStaticData>,
        scoring: Arc<Scoring>,
        options: Arc<Options>,
        progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
        rng: Rng,
    ) -> Self {
        let assignment_solver = AssignmentSolver::new_with_progress(
            input_data.clone(),
            cs_analysis.clone(),
            static_data.clone(),
            options.clone(),
            progress_sink.clone(),
        );
        Self {
            input_data,
            scoring,
            options,
            rng,
            progress_sink,
            assignment_count: 0,
            assignment_solver,
        }
    }

    pub(crate) fn solve(
        &mut self,
        scheduling: &Arc<Scheduling>,
        deadline: Option<SystemTime>,
    ) -> Solution {
        let mut best_solution = Solution::new(
            Some(scheduling.clone()),
            self.solve_assignment(scheduling, deadline),
        );
        if best_solution == Solution::Invalid {
            return Solution::Invalid;
        }

        let mut best_score = self.scoring.evaluate(&best_solution);
        if !best_score.is_finite() {
            return Solution::Invalid;
        }
        self.publish_best_solution(&best_solution, best_score);

        loop {
            if deadline.is_some_and(|deadline| SystemTime::now() >= deadline) {
                status::debug("Stopping hill climbing because the worker deadline was reached.");
                break;
            }

            let mut found_better_neighbor = false;
            for neighbor in self.pick_neighbors(
                best_solution
                    .scheduling()
                    .expect("solution requires scheduling"),
            ) {
                if deadline.is_some_and(|deadline| SystemTime::now() >= deadline) {
                    status::debug(
                        "Stopping hill climbing neighbor exploration at the worker deadline.",
                    );
                    return best_solution;
                }

                let neighbor_solution = Solution::new(
                    Some(neighbor.clone()),
                    self.solve_assignment(&neighbor, deadline),
                );
                let neighbor_score = self.scoring.evaluate(&neighbor_solution);
                if neighbor_score < best_score {
                    found_better_neighbor = true;
                    best_score = neighbor_score;
                    best_solution = neighbor_solution;
                    self.publish_best_solution(&best_solution, best_score);
                }
            }

            if !found_better_neighbor {
                break;
            }
        }

        best_solution
    }

    fn max_neighbor_key(&self) -> usize {
        self.input_data.choices.len() * self.input_data.slots.len()
    }

    fn solve_assignment(
        &mut self,
        scheduling: &Arc<Scheduling>,
        deadline: Option<SystemTime>,
    ) -> Option<Arc<crate::Assignment>> {
        let assignment = self
            .assignment_solver
            .solve_until(scheduling, deadline);
        self.assignment_count += 1;
        self.publish_assignment_progress();
        assignment
    }

    fn neighbor(&self, scheduling: &Scheduling, neighbor_key: usize) -> Arc<Scheduling> {
        let mut data = scheduling.data.clone();
        let choice_count = self.input_data.choices.len();
        let mut slot =
            i32::try_from(neighbor_key / choice_count).expect("neighbor key must fit in i32") - 1;
        let choice = neighbor_key % choice_count;

        if slot >= scheduling.slot_of(choice) {
            slot += 1;
        }
        data[choice] = slot;
        Arc::new(Scheduling::with_data(self.input_data.clone(), data))
    }

    fn random_swap_neighbor(&mut self, scheduling: &Scheduling) -> Arc<Scheduling> {
        let mut data = scheduling.data.clone();
        let mut swap_idx = vec![
            usize::try_from(self.rng.next_in_range(
                0,
                i32::try_from(data.len()).expect("data length must fit in i32"),
            ))
            .expect("random index must be non-negative"),
        ];

        while self.rng.next_in_range(0, 3) == 0 && swap_idx.len() < data.len() / 2 {
            let next_idx = loop {
                let candidate = usize::try_from(self.rng.next_in_range(
                    0,
                    i32::try_from(data.len()).expect("data length must fit in i32"),
                ))
                .expect("random index must be non-negative");
                if !swap_idx.contains(&candidate) {
                    break candidate;
                }
            };
            swap_idx.push(next_idx);
        }

        let mut carry = data[*swap_idx.last().expect("swap cycle must be non-empty")];
        for idx in swap_idx {
            std::mem::swap(&mut carry, &mut data[idx]);
        }

        Arc::new(Scheduling::with_data(self.input_data.clone(), data))
    }

    fn pick_neighbors(&mut self, scheduling: &Scheduling) -> Vec<Arc<Scheduling>> {
        let add_swap_neighbors =
            scheduling.input_data.choices.len() > 1 && scheduling.input_data.slots.len() > 1;
        let mut result = Vec::new();
        let mut neighbor_keys = (0..self.max_neighbor_key()).collect::<Vec<_>>();

        if self.max_neighbor_key()
            > usize::try_from(self.options.max_neighbors).expect("max_neighbors must be positive")
        {
            self.rng.shuffle(&mut neighbor_keys);
        }

        let max_neighbors =
            usize::try_from(self.options.max_neighbors).expect("max_neighbors must be positive");
        for (key_index, &neighbor_key) in neighbor_keys.iter().enumerate() {
            if result.len() >= max_neighbors {
                break;
            }
            if key_index > max_neighbors * 32 {
                break;
            }

            let next_neighbor = self.neighbor(scheduling, neighbor_key);
            if !next_neighbor.is_feasible() {
                continue;
            }
            result.push(next_neighbor);

            if add_swap_neighbors {
                let swap_neighbor = self.random_swap_neighbor(scheduling);
                if swap_neighbor.is_feasible() {
                    result.push(swap_neighbor);
                }
            }
        }

        if add_swap_neighbors && result.len() < max_neighbors {
            let amount = std::cmp::min(max_neighbors - result.len(), self.max_neighbor_key());
            for _ in 0..(amount * 32) {
                if result.len() >= max_neighbors {
                    break;
                }
                let swap_neighbor = self.random_swap_neighbor(scheduling);
                if swap_neighbor.is_feasible() {
                    result.push(swap_neighbor);
                }
            }
        }

        result
    }

    fn publish_assignment_progress(&self) {
        let Some(progress_sink) = &self.progress_sink else {
            return;
        };

        progress_sink
            .lock()
            .expect("progress mutex poisoned")
            .assignments = self.assignment_count;
    }

    fn publish_best_solution(&self, solution: &Solution, score: crate::Score) {
        let Some(progress_sink) = &self.progress_sink else {
            return;
        };

        let mut progress = progress_sink.lock().expect("progress mutex poisoned");
        if score < progress.best_score {
            progress.best_score = score;
            progress.best_solution = solution.clone();
        }
    }
}
