use std::time::SystemTime;

use crate::status;
use crate::{
    AssignmentSolver, Options, PreparedProblem, Rng, Scheduling, Solution,
};
use crate::shotgun_solver::ProgressReporter;

#[derive(Debug)]
pub(crate) struct HillClimbingSolver<'a> {
    problem: &'a PreparedProblem,
    options: &'a Options,
    progress_reporter: ProgressReporter,
    rng: Rng,
    pub(crate) assignment_count: usize,
    pub(crate) assignment_solver: AssignmentSolver<'a>,
}

impl<'a> HillClimbingSolver<'a> {
    pub(crate) fn new_with_rng(
        problem: &'a PreparedProblem,
        options: &'a Options,
        progress_reporter: ProgressReporter,
        rng: Rng,
    ) -> Self {
        let assignment_solver =
            AssignmentSolver::new_with_progress(problem, options, progress_reporter.clone());
        Self {
            problem,
            options,
            progress_reporter,
            rng,
            assignment_count: 0,
            assignment_solver,
        }
    }

    pub(crate) fn solve(
        &mut self,
        scheduling: &Scheduling,
        deadline: Option<SystemTime>,
    ) -> Solution {
        let mut best_solution = Solution::new(
            Some(scheduling.clone()),
            self.solve_assignment(scheduling, deadline),
        );
        if best_solution == Solution::Invalid {
            return Solution::Invalid;
        }

        let mut best_score = self.problem.scoring.evaluate(&self.problem.input_data, &best_solution);
        if !best_score.is_finite() {
            return Solution::Invalid;
        }
        self.progress_reporter
            .publish_best_solution(best_solution.clone(), best_score);

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

                let neighbor_solution =
                    Solution::new(Some(neighbor.clone()), self.solve_assignment(&neighbor, deadline));
                let neighbor_score =
                    self.problem.scoring.evaluate(&self.problem.input_data, &neighbor_solution);
                if neighbor_score < best_score {
                    found_better_neighbor = true;
                    best_score = neighbor_score;
                    best_solution = neighbor_solution.clone();
                    self.progress_reporter
                        .publish_best_solution(neighbor_solution, best_score);
                }
            }

            if !found_better_neighbor {
                break;
            }
        }

        best_solution
    }

    fn max_neighbor_key(&self) -> usize {
        self.problem.input_data.choices.len() * self.problem.input_data.slots.len()
    }

    fn solve_assignment(
        &mut self,
        scheduling: &Scheduling,
        deadline: Option<SystemTime>,
    ) -> Option<crate::Assignment> {
        let assignment = self.assignment_solver.solve_until(scheduling, deadline);
        self.assignment_count += 1;
        self.progress_reporter
            .publish_assignments(self.assignment_count);
        assignment
    }

    fn neighbor(&self, scheduling: &Scheduling, neighbor_key: usize) -> Scheduling {
        let mut data = scheduling.to_data();
        let choice_count = self.problem.input_data.choices.len();
        let mut slot = (neighbor_key / choice_count).checked_sub(1);
        let choice = neighbor_key % choice_count;

        if slot >= scheduling.slot_of(choice) {
            slot = slot.map(|slot| slot + 1).or(Some(0));
        }
        data[choice] = slot;
        Scheduling::with_data(&self.problem.input_data, data)
    }

    fn random_swap_neighbor(&mut self, scheduling: &Scheduling) -> Scheduling {
        let mut data = scheduling.to_data();
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

        Scheduling::with_data(&self.problem.input_data, data)
    }

    fn pick_neighbors(&mut self, scheduling: &Scheduling) -> Vec<Scheduling> {
        let add_swap_neighbors =
            self.problem.input_data.choices.len() > 1 && self.problem.input_data.slots.len() > 1;
        let mut result = Vec::new();
        let mut neighbor_keys = (0..self.max_neighbor_key()).collect::<Vec<_>>();

        if self.max_neighbor_key()
            > usize::try_from(self.options.max_neighbors).expect("max_neighbors must fit in usize")
        {
            self.rng.shuffle(&mut neighbor_keys);
        }

        let max_neighbors =
            usize::try_from(self.options.max_neighbors).expect("max_neighbors must fit in usize");
        for (key_index, &neighbor_key) in neighbor_keys.iter().enumerate() {
            if result.len() >= max_neighbors {
                break;
            }
            if key_index > max_neighbors * 32 {
                break;
            }

            let next_neighbor = self.neighbor(scheduling, neighbor_key);
            if !next_neighbor.is_feasible(&self.problem.input_data) {
                continue;
            }
            result.push(next_neighbor);

            if add_swap_neighbors {
                let swap_neighbor = self.random_swap_neighbor(scheduling);
                if swap_neighbor.is_feasible(&self.problem.input_data) {
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
                if swap_neighbor.is_feasible(&self.problem.input_data) {
                    result.push(swap_neighbor);
                }
            }
        }

        result
    }
}
