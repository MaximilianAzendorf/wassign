use std::collections::HashSet;
use std::time::{Duration, SystemTime};

use crate::shotgun_solver::ProgressReporter;
use crate::{
    Assignment, ConstraintType, MipFlow, MipFlowStaticData, Options, PreparedProblem, Scheduling,
};
use crate::status;

/// Computes an optimal assignment for a fixed scheduling.
#[derive(Debug)]
pub struct AssignmentSolver<'a> {
    problem: &'a PreparedProblem,
    options: &'a Options,
    progress_reporter: ProgressReporter,
    pub(crate) lp_count: usize,
    cached_schedule_state: Option<AssignmentScheduleState>,
    cached_assignment: Option<CachedAssignment>,
}

#[derive(Debug, Clone)]
struct AssignmentScheduleState {
    slots_of_choices: Vec<Option<usize>>,
    scheduled_choices_by_slot: Vec<Vec<usize>>,
    covered_choosers_by_slot: Vec<u32>,
    blocked_edges: HashSet<(usize, usize)>,
}

#[derive(Debug, Clone)]
struct CachedAssignment {
    slots_of_choices: Vec<Option<usize>>,
    assignment: Assignment,
    is_optimal: bool,
}

#[derive(Debug)]
enum LimitSolveResult {
    Feasible(Assignment),
    Infeasible,
    Incomplete,
}

impl<'a> AssignmentSolver<'a> {
    /// Creates a solver for the assignment stage of the prepared problem.
    #[must_use]
    pub fn new(problem: &'a PreparedProblem, options: &'a Options) -> Self {
        Self::new_with_progress(problem, options, ProgressReporter::dummy())
    }

    pub(crate) fn new_with_progress(
        problem: &'a PreparedProblem,
        options: &'a Options,
        progress_reporter: ProgressReporter,
    ) -> Self {
        Self {
            problem,
            options,
            progress_reporter,
            lp_count: 0,
            cached_schedule_state: None,
            cached_assignment: None,
        }
    }

    /// Solves the assignment stage for a fixed scheduling.
    #[must_use]
    pub fn solve(&mut self, scheduling: &Scheduling) -> Option<Assignment> {
        self.solve_until(scheduling, None)
    }

    pub(crate) fn solve_until(
        &mut self,
        scheduling: &Scheduling,
        deadline: Option<SystemTime>,
    ) -> Option<Assignment> {
        let input_data = &self.problem.input_data;
        let levels = input_data.preference_levels.clone();
        let start = levels
            .iter()
            .position(|&level| level >= self.problem.critical_set_analysis.preference_bound())
            .unwrap_or_else(|| levels.len().saturating_sub(1));
        let schedule_state = self.prepare_schedule_state(scheduling);

        if let Some(cached) = &self.cached_assignment
            && cached.slots_of_choices == schedule_state.slots_of_choices
            && cached.is_optimal
        {
            return Some(cached.assignment.clone());
        }

        status::debug(&format!(
            "Starting assignment solve with preference search start index {start} and {} level(s).",
            levels.len()
        ));

        if self.options.greedy {
            status::debug("Greedy mode enabled; solving with maximum preference limit.");
            let result = self.solve_with_limit(&schedule_state, input_data.max_preference, deadline);
            let (assignment, is_optimal) = match result {
                LimitSolveResult::Feasible(assignment) => (Some(assignment), true),
                LimitSolveResult::Infeasible => (None, false),
                LimitSolveResult::Incomplete => (None, false),
            };
            return self.finish_assignment_result(
                assignment,
                schedule_state.slots_of_choices,
                is_optimal,
            );
        }

        let mut low = start;
        let mut high = levels.len().checked_sub(1)?;
        let mut best = None;
        let mut is_optimal = true;

        while low <= high {
            let mid = low + (high - low) / 2;
            let preference_limit = levels[mid];
            status::trace(&format!(
                "Trying assignment solve with preference limit {preference_limit} (range {low}..={high})."
            ));
            match self.solve_with_limit(&schedule_state, preference_limit, deadline) {
                LimitSolveResult::Feasible(assignment) => {
                    status::debug(&format!(
                        "Assignment feasible with preference limit {preference_limit}."
                    ));
                    best = Some(assignment);
                    if mid == start {
                        break;
                    }
                    high = mid.saturating_sub(1);
                }
                LimitSolveResult::Infeasible => {
                    status::trace(&format!(
                        "Assignment infeasible with preference limit {preference_limit}."
                    ));
                    low = mid + 1;
                }
                LimitSolveResult::Incomplete => {
                    status::debug(&format!(
                        "Assignment search could not classify preference limit {preference_limit}; continuing with more relaxed limits."
                    ));
                    is_optimal = false;
                    low = mid + 1;
                }
            }
        }

        self.finish_assignment_result(best, schedule_state.slots_of_choices, is_optimal)
    }

    fn finish_assignment_result(
        &mut self,
        assignment: Option<Assignment>,
        slots_of_choices: Vec<Option<usize>>,
        is_optimal: bool,
    ) -> Option<Assignment> {
        if let Some(assignment) = assignment {
            if is_optimal {
                self.cached_assignment = Some(CachedAssignment {
                    slots_of_choices,
                    assignment: assignment.clone(),
                    is_optimal,
                });
            }
            Some(assignment)
        } else {
            None
        }
    }

    fn build_schedule_state_from_scratch(
        &self,
        scheduling: &Scheduling,
    ) -> AssignmentScheduleState {
        let input_data = &self.problem.input_data;
        let slots_of_choices = scheduling.to_data();
        let mut scheduled_choices_by_slot = vec![Vec::new(); input_data.slots.len()];
        let mut covered_choosers_by_slot = vec![0_u32; input_data.slots.len()];

        for (choice, &slot) in slots_of_choices.iter().enumerate() {
            let Some(slot) = slot else {
                continue;
            };
            scheduled_choices_by_slot[slot].push(choice);
            covered_choosers_by_slot[slot] += input_data.choices[choice].min;
        }

        let blocked_edges =
            self.build_blocked_constraint_edges(&slots_of_choices, &scheduled_choices_by_slot);

        AssignmentScheduleState {
            slots_of_choices,
            scheduled_choices_by_slot,
            covered_choosers_by_slot,
            blocked_edges,
        }
    }

    fn update_schedule_state(
        &self,
        previous: &AssignmentScheduleState,
        scheduling: &Scheduling,
    ) -> AssignmentScheduleState {
        let input_data = &self.problem.input_data;
        let slots_of_choices = scheduling.to_data();
        let mut next = previous.clone();

        for (choice, (&old_slot, &new_slot)) in previous
            .slots_of_choices
            .iter()
            .zip(slots_of_choices.iter())
            .enumerate()
        {
            if old_slot == new_slot {
                continue;
            }

            if let Some(slot) = old_slot {
                if let Some(index) = next.scheduled_choices_by_slot[slot]
                    .iter()
                    .position(|&scheduled_choice| scheduled_choice == choice)
                {
                    next.scheduled_choices_by_slot[slot].swap_remove(index);
                }
                next.covered_choosers_by_slot[slot] -= input_data.choices[choice].min;
            }

            if let Some(slot) = new_slot {
                next.scheduled_choices_by_slot[slot].push(choice);
                next.covered_choosers_by_slot[slot] += input_data.choices[choice].min;
            }
        }

        next.slots_of_choices = slots_of_choices;
        next.blocked_edges = self.build_blocked_constraint_edges(
            &next.slots_of_choices,
            &next.scheduled_choices_by_slot,
        );
        next
    }

    fn prepare_schedule_state(&mut self, scheduling: &Scheduling) -> AssignmentScheduleState {
        let next = if let Some(previous) = &self.cached_schedule_state {
            self.update_schedule_state(previous, scheduling)
        } else {
            self.build_schedule_state_from_scratch(scheduling)
        };
        self.cached_schedule_state = Some(next.clone());
        next
    }

    fn build_blocked_constraint_edges(
        &self,
        slots_of_choices: &[Option<usize>],
        scheduled_choices_by_slot: &[Vec<usize>],
    ) -> HashSet<(usize, usize)> {
        let input_data = &self.problem.input_data;
        let static_data = &self.problem.static_flow_data;
        let mut blocked_edges = HashSet::new();

        for constraint in &static_data.constraints {
            match constraint.kind {
                ConstraintType::ChooserIsInChoice => {
                    let constrained_choice = constraint.other_choice();
                    let Some(slot_index) = slots_of_choices[constrained_choice] else {
                        continue;
                    };
                    let from = static_data.chooser_nodes[constraint.left][slot_index];

                    for &choice in &scheduled_choices_by_slot[slot_index] {
                        if choice == constrained_choice {
                            continue;
                        }
                        blocked_edges.insert((from, static_data.choice_nodes[choice]));
                    }
                }
                ConstraintType::ChooserIsNotInChoice => {
                    let blocked_choice = constraint.other_choice();
                    let to = static_data.choice_nodes[blocked_choice];
                    for slot in 0..input_data.slots.len() {
                        blocked_edges
                            .insert((static_data.chooser_nodes[constraint.left][slot], to));
                    }
                }
                ConstraintType::ChoicesHaveSameChoosers
                | ConstraintType::ChoosersHaveSameChoices => {}
                kind => panic!(
                    "This kind of constraint is not compatible with the min cost flow solver: {kind:?}."
                ),
            }
        }

        blocked_edges
    }

    fn create_edge_groups(
        &self,
        schedule_state: &AssignmentScheduleState,
        flow: &mut MipFlow<u64, u64>,
    ) {
        let input_data = &self.problem.input_data;
        let static_data = &self.problem.static_flow_data;
        for constraint in &input_data.assignment_constraints {
            if constraint.kind != ConstraintType::ChoosersHaveSameChoices {
                continue;
            }

            let other_chooser = constraint.other_chooser();
            for slot in 0..input_data.slots.len() {
                for &choice in &schedule_state.scheduled_choices_by_slot[slot] {
                    let from1 = static_data.chooser_nodes[constraint.left][slot];
                    let to1 = static_data.choice_nodes[choice];
                    let from2 = static_data.chooser_nodes[other_chooser][slot];
                    let to2 = static_data.choice_nodes[choice];
                    flow.create_edge_group_or_block_edges([
                        MipFlowStaticData::edge_id(from1, to1),
                        MipFlowStaticData::edge_id(from2, to2),
                    ]);
                }
            }
        }

        for group in &input_data.dependent_choice_groups {
            if group.len() == 1 {
                continue;
            }

            for chooser in 0..input_data.choosers.len() {
                let mut edge_group = Vec::new();
                for choice in group.iter().copied() {
                    let Some(slot_index) = schedule_state.slots_of_choices[choice] else {
                        continue;
                    };
                    let from = static_data.chooser_nodes[chooser][slot_index];
                    let to = static_data.choice_nodes[choice];
                    edge_group.push(MipFlowStaticData::edge_id(from, to));
                }
                flow.create_edge_group_or_block_edges(edge_group);
            }
        }
    }

    fn solve_with_limit(
        &mut self,
        schedule_state: &AssignmentScheduleState,
        preference_limit: u32,
        deadline: Option<SystemTime>,
    ) -> LimitSolveResult {
        let input_data = &self.problem.input_data;
        let static_data = &self.problem.static_flow_data;
        if deadline.is_some_and(|deadline| deadline <= SystemTime::now()) {
            status::debug("Skipping assignment solve because the worker deadline is exhausted.");
            return LimitSolveResult::Incomplete;
        }

        let mut flow = static_data.base_flow.clone();
        let mut blocked_edges = schedule_state.blocked_edges.clone();
        blocked_edges.extend(static_data.blocked_edges.iter().copied());

        for chooser in 0..input_data.choosers.len() {
            for slot in 0..input_data.slots.len() {
                flow.set_supply(static_data.chooser_nodes[chooser][slot], 1);
            }
        }

        for (choice, &slot) in schedule_state.slots_of_choices.iter().enumerate() {
            if slot.is_none() {
                continue;
            }
            flow.set_supply(
                static_data.choice_nodes[choice],
                -i32::try_from(input_data.choices[choice].min).expect("choice min must fit in i32"),
            );
        }

        for slot in 0..input_data.slots.len() {
            flow.set_supply(
                static_data.slot_nodes[slot],
                -(i32::try_from(input_data.choosers.len()).expect("chooser count must fit in i32")
                    - i32::try_from(schedule_state.covered_choosers_by_slot[slot])
                        .expect("covered chooser count must fit in i32")),
            );
        }

        for chooser in 0..input_data.choosers.len() {
            for (slot, choices) in schedule_state.scheduled_choices_by_slot.iter().enumerate() {
                let from = static_data.chooser_nodes[chooser][slot];
                for &choice in choices {
                    if input_data.choosers[chooser].preferences[choice] > preference_limit {
                        continue;
                    }

                    let to = static_data.choice_nodes[choice];
                    if blocked_edges.contains(&(from, to)) {
                        continue;
                    }
                    let cost = ((f64::from(input_data.choosers[chooser].preferences[choice]) + 1.0)
                        .powf(self.options.preference_exponent))
                        as i64;
                    flow.add_keyed_edge(MipFlowStaticData::edge_id(from, to), from, to, 1, cost);
                }
            }
        }

        for (choice, &slot_index) in schedule_state.slots_of_choices.iter().enumerate() {
            let Some(slot_index) = slot_index else {
                continue;
            };
            flow.add_keyed_edge(
                MipFlowStaticData::edge_id(
                    static_data.choice_nodes[choice],
                    static_data.slot_nodes[slot_index],
                ),
                static_data.choice_nodes[choice],
                static_data.slot_nodes[slot_index],
                i32::try_from(input_data.choices[choice].max - input_data.choices[choice].min)
                    .expect("choice width must fit in i32"),
                0,
            );
        }

        self.create_edge_groups(schedule_state, &mut flow);

        self.lp_count += 1;
        self.progress_reporter.publish_lp(self.lp_count);
        status::trace(&format!(
            "Solving assignment LP/MIP instance #{} with {} node(s) and {} edge(s).",
            self.lp_count,
            flow.node_count(),
            flow.edge_count()
        ));
        let remaining = remaining_duration(deadline);
        if remaining.is_some_and(|remaining| remaining.is_zero()) {
            status::debug("Skipping assignment flow solve because no time remains.");
            return LimitSolveResult::Incomplete;
        }
        if !flow.solve(remaining) {
            if deadline.is_some_and(|deadline| SystemTime::now() >= deadline) {
                status::debug(
                    "Assignment flow solve stopped before producing a feasible solution.",
                );
                return LimitSolveResult::Incomplete;
            }
            status::trace("Assignment flow solve reported infeasibility.");
            return LimitSolveResult::Infeasible;
        }

        let mut data = vec![vec![usize::MAX; input_data.slots.len()]; input_data.choosers.len()];
        for (chooser, slots) in data.iter_mut().enumerate() {
            for (slot, choice_slot) in slots.iter_mut().enumerate() {
                let from = static_data.chooser_nodes[chooser][slot];
                for &choice in &schedule_state.scheduled_choices_by_slot[slot] {
                    let to = static_data.choice_nodes[choice];
                    if flow.solution_value_at(&MipFlowStaticData::edge_id(from, to)) == 1 {
                        *choice_slot = choice;
                        break;
                    }
                }
                assert_ne!(
                    *choice_slot,
                    usize::MAX,
                    "each chooser/slot must be assigned a choice"
                );
            }
        }

        status::trace("Assignment flow solve succeeded.");
        LimitSolveResult::Feasible(Assignment::new(input_data, data))
    }
}

fn remaining_duration(deadline: Option<SystemTime>) -> Option<Duration> {
    deadline.map(|deadline| {
        deadline
            .duration_since(SystemTime::now())
            .unwrap_or(Duration::ZERO)
    })
}
