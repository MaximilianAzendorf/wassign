use std::collections::BTreeSet;
use std::time::{Duration, SystemTime};

use crate::shotgun_solver::ProgressReporter;
use crate::{
    Assignment, ConstraintType, MipFlow, MipFlowStaticData, Options, PreparedProblem, Scheduling,
};
use crate::{constraints, status};

/// Computes an optimal assignment for a fixed scheduling.
#[derive(Debug)]
pub struct AssignmentSolver<'a> {
    problem: &'a PreparedProblem,
    options: &'a Options,
    progress_reporter: ProgressReporter,
    pub(crate) lp_count: usize,
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
        status::debug(&format!(
            "Starting assignment solve with preference search start index {start} and {} level(s).",
            levels.len()
        ));

        if self.options.greedy {
            status::debug("Greedy mode enabled; solving with maximum preference limit.");
            return self.solve_with_limit(scheduling, input_data.max_preference, deadline);
        }

        let mut low = start;
        let mut high = levels.len().checked_sub(1)?;
        let mut best = None;

        while low <= high {
            let mid = low + (high - low) / 2;
            let preference_limit = levels[mid];
            status::trace(&format!(
                "Trying assignment solve with preference limit {preference_limit} (range {low}..={high})."
            ));
            let assignment = self.solve_with_limit(scheduling, preference_limit, deadline);
            if assignment.is_some() {
                status::debug(&format!(
                    "Assignment feasible with preference limit {preference_limit}."
                ));
                best = assignment;
                if mid == start {
                    break;
                }
                high = mid.saturating_sub(1);
            } else {
                status::trace(&format!(
                    "Assignment infeasible with preference limit {preference_limit}."
                ));
                low = mid + 1;
            }
        }

        best
    }

    fn get_blocked_constraint_edges(&self, scheduling: &Scheduling) -> BTreeSet<(usize, usize)> {
        let input_data = &self.problem.input_data;
        let static_data = &self.problem.static_flow_data;
        let mut blocked_edges = BTreeSet::new();

        for constraint in &static_data.constraints {
            match constraint.kind {
                ConstraintType::ChooserIsInChoice => {
                    let constrained_choice = constraint.other_choice();
                    let slot = scheduling.slot_of(constrained_choice);
                    let Some(slot_index) = slot else {
                        continue;
                    };
                    let from = static_data.base_flow.node_map
                        [&MipFlowStaticData::node_chooser(constraint.left, slot_index)];

                    for choice in 0..input_data.choices.len() {
                        if choice == constrained_choice || scheduling.slot_of(choice) != slot {
                            continue;
                        }
                        let to =
                            static_data.base_flow.node_map[&MipFlowStaticData::node_choice(choice)];
                        blocked_edges.insert((from, to));
                    }
                }
                ConstraintType::ChooserIsNotInChoice => {
                    let blocked_choice = constraint.other_choice();
                    let to = static_data.base_flow.node_map
                        [&MipFlowStaticData::node_choice(blocked_choice)];
                    for slot in 0..input_data.slots.len() {
                        let from = static_data.base_flow.node_map
                            [&MipFlowStaticData::node_chooser(constraint.left, slot)];
                        blocked_edges.insert((from, to));
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

    fn create_edge_groups(&self, scheduling: &Scheduling, flow: &mut MipFlow<u64, u64>) {
        let input_data = &self.problem.input_data;
        let static_data = &self.problem.static_flow_data;
        for constraint in &input_data.assignment_constraints {
            if constraint.kind != ConstraintType::ChoosersHaveSameChoices {
                continue;
            }

            let other_chooser = constraint.other_chooser();
            for slot in 0..input_data.slots.len() {
                for choice in 0..input_data.choices.len() {
                    let from1 = static_data.base_flow.node_map
                        [&MipFlowStaticData::node_chooser(constraint.left, slot)];
                    let to1 =
                        static_data.base_flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    let from2 = static_data.base_flow.node_map
                        [&MipFlowStaticData::node_chooser(other_chooser, slot)];
                    let to2 =
                        static_data.base_flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    flow.create_edge_group_or_block_edges([
                        MipFlowStaticData::edge_id(from1, to1),
                        MipFlowStaticData::edge_id(from2, to2),
                    ]);
                }
            }
        }

        for group in constraints::get_dependent_choices(
            &input_data.assignment_constraints,
            input_data.choices.len(),
        ) {
            if group.len() == 1 {
                continue;
            }

            for chooser in 0..input_data.choosers.len() {
                let mut edge_group = Vec::new();
                for choice in group.iter().copied() {
                    let Some(slot_index) = scheduling.slot_of(choice) else {
                        continue;
                    };
                    let from = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot_index)];
                    let to = flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    edge_group.push(MipFlowStaticData::edge_id(from, to));
                }
                flow.create_edge_group_or_block_edges(edge_group);
            }
        }
    }

    fn solve_with_limit(
        &mut self,
        scheduling: &Scheduling,
        preference_limit: u32,
        deadline: Option<SystemTime>,
    ) -> Option<Assignment> {
        let input_data = &self.problem.input_data;
        let static_data = &self.problem.static_flow_data;
        if deadline.is_some_and(|deadline| deadline <= SystemTime::now()) {
            status::debug("Skipping assignment solve because the worker deadline is exhausted.");
            return None;
        }

        let mut flow = static_data.base_flow.clone();
        let mut blocked_edges = self.get_blocked_constraint_edges(scheduling);
        blocked_edges.extend(static_data.blocked_edges.iter().copied());

        for chooser in 0..input_data.choosers.len() {
            for slot in 0..input_data.slots.len() {
                let node = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot)];
                flow.set_supply(node, 1);
            }
        }

        for choice in 0..input_data.choices.len() {
            if scheduling.slot_of(choice).is_none() {
                continue;
            }
            let node = flow.node_map[&MipFlowStaticData::node_choice(choice)];
            flow.set_supply(
                node,
                -i32::try_from(input_data.choices[choice].min).expect("choice min must fit in i32"),
            );
        }

        for slot in 0..input_data.slots.len() {
            let mut covered_choosers = 0_u32;
            for choice in 0..input_data.choices.len() {
                if scheduling.slot_of(choice) == Some(slot) {
                    covered_choosers += input_data.choices[choice].min;
                }
            }
            let node = flow.node_map[&MipFlowStaticData::node_slot(slot)];
            flow.set_supply(
                node,
                -(i32::try_from(input_data.choosers.len()).expect("chooser count must fit in i32")
                    - i32::try_from(covered_choosers)
                        .expect("covered chooser count must fit in i32")),
            );
        }

        for chooser in 0..input_data.choosers.len() {
            for slot in 0..input_data.slots.len() {
                for choice in 0..input_data.choices.len() {
                    if scheduling.slot_of(choice) != Some(slot)
                        || input_data.choosers[chooser].preferences[choice] > preference_limit
                    {
                        continue;
                    }

                    let from = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot)];
                    let to = flow.node_map[&MipFlowStaticData::node_choice(choice)];
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

        for choice in 0..input_data.choices.len() {
            let Some(slot_index) = scheduling.slot_of(choice) else {
                continue;
            };
            let from = flow.node_map[&MipFlowStaticData::node_choice(choice)];
            let to = flow.node_map[&MipFlowStaticData::node_slot(slot_index)];
            flow.add_keyed_edge(
                MipFlowStaticData::edge_id(from, to),
                from,
                to,
                i32::try_from(input_data.choices[choice].max - input_data.choices[choice].min)
                    .expect("choice width must fit in i32"),
                0,
            );
        }

        self.create_edge_groups(scheduling, &mut flow);

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
            return None;
        }
        if !flow.solve(remaining) {
            if deadline.is_some() {
                status::debug(
                    "Assignment flow solve stopped before producing a feasible solution.",
                );
            }
            status::trace("Assignment flow solve reported infeasibility.");
            return None;
        }

        let mut data = vec![vec![usize::MAX; input_data.slots.len()]; input_data.choosers.len()];
        for (chooser, slots) in data.iter_mut().enumerate() {
            for (slot, choice_slot) in slots.iter_mut().enumerate() {
                for choice in 0..input_data.choices.len() {
                    let from = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot)];
                    let to = flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    if flow.solution_value_at(&MipFlowStaticData::edge_id(from, to)) == 1 {
                        *choice_slot = choice;
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
        Some(Assignment::new(input_data, data))
    }
}

fn remaining_duration(deadline: Option<SystemTime>) -> Option<Duration> {
    deadline.map(|deadline| {
        deadline
            .duration_since(SystemTime::now())
            .unwrap_or(Duration::ZERO)
    })
}
