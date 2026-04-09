#![expect(
    clippy::cast_possible_truncation,
    clippy::needless_pass_by_value,
    clippy::too_many_lines,
    reason = "the assignment solver uses index-heavy graph assembly and one large branchy solve loop"
)]

use std::collections::BTreeSet;
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::{
    Assignment, ConstraintType, Constraints, CriticalSetAnalysis, InputData, MipFlow, MipFlowStaticData, Options,
    Scheduling, Status,
};
use crate::shotgun_solver::ShotgunSolverProgress;

/// Computes an optimal assignment for a fixed scheduling.
#[derive(Debug)]
pub struct AssignmentSolver {
    input_data: Arc<InputData>,
    cs_analysis: Arc<CriticalSetAnalysis>,
    static_data: Arc<MipFlowStaticData>,
    options: Arc<Options>,
    progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
    pub(crate) lp_count: usize,
}

impl AssignmentSolver {
    /// Creates a solver for the assignment stage.
    #[must_use]
    pub fn new(
        input_data: Arc<InputData>,
        cs_analysis: Arc<CriticalSetAnalysis>,
        static_data: Arc<MipFlowStaticData>,
        options: Arc<Options>,
    ) -> Self {
        Self::new_with_progress(input_data, cs_analysis, static_data, options, None)
    }

    pub(crate) fn new_with_progress(
        input_data: Arc<InputData>,
        cs_analysis: Arc<CriticalSetAnalysis>,
        static_data: Arc<MipFlowStaticData>,
        options: Arc<Options>,
        progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
    ) -> Self {
        Self {
            input_data,
            cs_analysis,
            static_data,
            options,
            progress_sink,
            lp_count: 0,
        }
    }

    /// Solves the assignment stage for the given scheduling.
    ///
    /// Returns `None` when no feasible assignment exists.
    #[must_use]
    pub fn solve(&mut self, scheduling: Arc<Scheduling>) -> Option<Arc<Assignment>> {
        self.solve_until(scheduling, None)
    }

    pub(crate) fn solve_until(
        &mut self,
        scheduling: Arc<Scheduling>,
        deadline: Option<SystemTime>,
    ) -> Option<Arc<Assignment>> {
        let levels = self.input_data.preference_levels.clone();
        let start = levels
            .iter()
            .position(|&level| level >= self.cs_analysis.preference_bound())
            .unwrap_or_else(|| levels.len().saturating_sub(1));
        Status::debug(&format!(
            "Starting assignment solve with preference search start index {start} and {} level(s).",
            levels.len()
        ));

        if self.options.greedy {
            Status::debug("Greedy mode enabled; solving with maximum preference limit.");
            return self.solve_with_limit(&scheduling, self.input_data.max_preference(), deadline);
        }

        let mut low = start;
        let mut high = levels.len().checked_sub(1)?;
        let mut best = None;

        while low <= high {
            let mid = low + (high - low) / 2;
            let preference_limit = levels[mid];
            Status::trace(&format!(
                "Trying assignment solve with preference limit {preference_limit} (range {low}..={high})."
            ));
            let assignment = self.solve_with_limit(&scheduling, preference_limit, deadline);
            if assignment.is_some() {
                Status::debug(&format!("Assignment feasible with preference limit {preference_limit}."));
                best = assignment;
                if mid == start {
                    break;
                }
                high = mid.saturating_sub(1);
            } else {
                Status::trace(&format!("Assignment infeasible with preference limit {preference_limit}."));
                low = mid + 1;
            }
        }

        best
    }

    fn get_blocked_constraint_edges(&self, scheduling: &Scheduling) -> BTreeSet<(usize, usize)> {
        let mut blocked_edges = BTreeSet::new();

        for constraint in &self.static_data.constraints {
            match constraint.kind {
                ConstraintType::ChooserIsInChoice => {
                    let constrained_choice =
                        usize::try_from(constraint.right).expect("choice index must be non-negative");
                    let slot = scheduling.slot_of(constrained_choice);
                    if slot < 0 {
                        continue;
                    }
                    let slot_index = usize::try_from(slot).expect("slot id must be non-negative");
                    let from = self.static_data.base_flow.node_map
                        [&MipFlowStaticData::node_chooser(constraint.left, slot_index)];

                    for choice in 0..self.input_data.choice_count() {
                        if choice == constrained_choice || scheduling.slot_of(choice) != slot {
                            continue;
                        }
                        let to = self.static_data.base_flow.node_map[&MipFlowStaticData::node_choice(choice)];
                        blocked_edges.insert((from, to));
                    }
                }
                ConstraintType::ChooserIsNotInChoice => {
                    let blocked_choice =
                        usize::try_from(constraint.right).expect("choice index must be non-negative");
                    let to = self.static_data.base_flow.node_map[&MipFlowStaticData::node_choice(blocked_choice)];
                    for slot in 0..self.input_data.slot_count() {
                        let from = self.static_data.base_flow.node_map
                            [&MipFlowStaticData::node_chooser(constraint.left, slot)];
                        blocked_edges.insert((from, to));
                    }
                }
                ConstraintType::ChoicesHaveSameChoosers | ConstraintType::ChoosersHaveSameChoices => {}
                kind => panic!("This kind of constraint is not compatible with the min cost flow solver: {kind:?}."),
            }
        }

        blocked_edges
    }

    fn create_edge_groups(&self, scheduling: &Scheduling, flow: &mut MipFlow<u64, u64>) {
        for constraint in self.input_data.assignment_constraints() {
            if constraint.kind != ConstraintType::ChoosersHaveSameChoices {
                continue;
            }

            let other_chooser = usize::try_from(constraint.right).expect("chooser index must be non-negative");
            for slot in 0..self.input_data.slot_count() {
                for choice in 0..self.input_data.choice_count() {
                    let from1 = self.static_data.base_flow.node_map
                        [&MipFlowStaticData::node_chooser(constraint.left, slot)];
                    let to1 = self.static_data.base_flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    let from2 = self.static_data.base_flow.node_map
                        [&MipFlowStaticData::node_chooser(other_chooser, slot)];
                    let to2 = self.static_data.base_flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    flow.create_edge_group_or_block_edges([
                        MipFlowStaticData::edge_id(from1, to1),
                        MipFlowStaticData::edge_id(from2, to2),
                    ]);
                }
            }
        }

        for group in Constraints::get_dependent_choices(
            self.input_data.assignment_constraints(),
            self.input_data.choice_count(),
        ) {
            if group.len() == 1 {
                continue;
            }

            for chooser in 0..self.input_data.chooser_count() {
                let mut edge_group = Vec::new();
                for choice in group.iter().copied() {
                    let slot = scheduling.slot_of(choice);
                    if slot < 0 {
                        continue;
                    }

                    let slot_index = usize::try_from(slot).expect("slot id must be non-negative");
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
        preference_limit: i32,
        deadline: Option<SystemTime>,
    ) -> Option<Arc<Assignment>> {
        if deadline.is_some_and(|deadline| deadline <= SystemTime::now()) {
            Status::debug("Skipping assignment solve because the worker deadline is exhausted.");
            return None;
        }

        let mut flow = self.static_data.base_flow.clone();
        let mut blocked_edges = self.get_blocked_constraint_edges(scheduling);
        blocked_edges.extend(self.static_data.blocked_edges.iter().copied());

        for chooser in 0..self.input_data.chooser_count() {
            for slot in 0..self.input_data.slot_count() {
                let node = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot)];
                flow.set_supply(node, 1);
            }
        }

        for choice in 0..self.input_data.choice_count() {
            if scheduling.slot_of(choice) == Scheduling::NOT_SCHEDULED {
                continue;
            }
            let node = flow.node_map[&MipFlowStaticData::node_choice(choice)];
            flow.set_supply(node, -self.input_data.choices[choice].min);
        }

        for slot in 0..self.input_data.slot_count() {
            let mut covered_choosers = 0_i32;
            for choice in 0..self.input_data.choice_count() {
                if scheduling.slot_of(choice) == i32::try_from(slot).expect("slot index must fit in i32") {
                    covered_choosers += self.input_data.choices[choice].min;
                }
            }
            let node = flow.node_map[&MipFlowStaticData::node_slot(slot)];
            flow.set_supply(
                node,
                -(i32::try_from(self.input_data.chooser_count()).expect("chooser count must fit in i32")
                    - covered_choosers),
            );
        }

        for chooser in 0..self.input_data.chooser_count() {
            for slot in 0..self.input_data.slot_count() {
                for choice in 0..self.input_data.choice_count() {
                    if scheduling.slot_of(choice) != i32::try_from(slot).expect("slot index must fit in i32")
                        || self.input_data.choosers[chooser].preferences[choice] > preference_limit
                    {
                        continue;
                    }

                    let from = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot)];
                    let to = flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    if blocked_edges.contains(&(from, to)) {
                        continue;
                    }
                    let cost =
                        ((f64::from(self.input_data.choosers[chooser].preferences[choice]) + 1.0)
                            .powf(self.options.preference_exponent)) as i64;
                    flow.add_keyed_edge(MipFlowStaticData::edge_id(from, to), from, to, 1, cost);
                }
            }
        }

        for choice in 0..self.input_data.choice_count() {
            let slot = scheduling.slot_of(choice);
            if slot < 0 {
                continue;
            }

            let slot_index = usize::try_from(slot).expect("slot id must be non-negative");
            let from = flow.node_map[&MipFlowStaticData::node_choice(choice)];
            let to = flow.node_map[&MipFlowStaticData::node_slot(slot_index)];
            flow.add_keyed_edge(
                MipFlowStaticData::edge_id(from, to),
                from,
                to,
                self.input_data.choices[choice].max - self.input_data.choices[choice].min,
                0,
            );
        }

        self.create_edge_groups(scheduling, &mut flow);

        self.lp_count += 1;
        self.publish_lp_progress();
        Status::trace(&format!(
            "Solving assignment LP/MIP instance #{} with {} node(s) and {} edge(s).",
            self.lp_count,
            flow.node_count(),
            flow.edge_count()
        ));
        let remaining = remaining_duration(deadline);
        if remaining.is_some_and(|remaining| remaining.is_zero()) {
            Status::debug("Skipping assignment flow solve because no time remains.");
            return None;
        }
        if !flow.solve(remaining) {
            if deadline.is_some() {
                Status::debug("Assignment flow solve stopped before producing a feasible solution.");
            }
            Status::trace("Assignment flow solve reported infeasibility.");
            return None;
        }

        let mut data = vec![vec![usize::MAX; self.input_data.slot_count()]; self.input_data.chooser_count()];
        for (chooser, slots) in data.iter_mut().enumerate() {
            for (slot, choice_slot) in slots.iter_mut().enumerate() {
                for choice in 0..self.input_data.choice_count() {
                    let from = flow.node_map[&MipFlowStaticData::node_chooser(chooser, slot)];
                    let to = flow.node_map[&MipFlowStaticData::node_choice(choice)];
                    if flow.solution_value_at(&MipFlowStaticData::edge_id(from, to)) == 1 {
                        *choice_slot = choice;
                    }
                }
                assert_ne!(*choice_slot, usize::MAX, "each chooser/slot must be assigned a choice");
            }
        }

        Status::trace("Assignment flow solve succeeded.");
        Some(Arc::new(Assignment::new(self.input_data.clone(), data)))
    }
}

impl AssignmentSolver {
    fn publish_lp_progress(&self) {
        let Some(progress_sink) = &self.progress_sink else {
            return;
        };

        progress_sink.lock().expect("progress mutex poisoned").lp = self.lp_count;
    }
}

fn remaining_duration(deadline: Option<SystemTime>) -> Option<Duration> {
    deadline.map(|deadline| deadline.duration_since(SystemTime::now()).unwrap_or(Duration::ZERO))
}
