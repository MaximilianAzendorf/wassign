use std::cmp::{Reverse, max, min};
use std::time::{Duration, SystemTime};

use crate::shotgun_solver::ProgressReporter;
use crate::status;
use crate::util::{riffle_shuffle, time_never, time_now};
use crate::{ConstraintType, CriticalSet, Options, PreparedProblem, Rng, Scheduling};

/// Enumerates feasible schedulings for a given input.
#[derive(Debug)]
pub struct SchedulingSolver<'a> {
    problem: &'a PreparedProblem,
    options: &'a Options,
    current_solution: Option<Scheduling>,
    has_solution: bool,
    progress_reporter: ProgressReporter,
    rng: Rng,
    pub(crate) current_depth: usize,
    pub(crate) max_depth_reached: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum SchedulingDecision {
    Undecided,
    Omitted,
    Slot(usize),
}

#[derive(Debug, Clone)]
struct SchedulingSearchState {
    decisions: Vec<SchedulingDecision>,
    slot_min: Vec<u32>,
    slot_max: Vec<u32>,
    slot_choice_count: Vec<u32>,
}

impl SchedulingDecision {
    fn slot(self) -> Option<usize> {
        match self {
            Self::Undecided | Self::Omitted => None,
            Self::Slot(slot) => Some(slot),
        }
    }
}

impl SchedulingSearchState {
    fn new(choice_count: usize, slot_count: usize) -> Self {
        Self {
            decisions: vec![SchedulingDecision::Undecided; choice_count],
            slot_min: vec![0; slot_count],
            slot_max: vec![0; slot_count],
            slot_choice_count: vec![0; slot_count],
        }
    }
}

impl<'a> SchedulingSolver<'a> {
    pub(crate) const PREF_RELAXATION: i32 = 10;

    /// Creates a new scheduling solver for the prepared problem.
    #[must_use]
    pub fn new(problem: &'a PreparedProblem, options: &'a Options) -> Self {
        Self::new_with_progress(
            problem,
            options,
            ProgressReporter::dummy(),
            Rng::from_global_seed(),
        )
    }

    pub(crate) fn new_with_rng(
        problem: &'a PreparedProblem,
        options: &'a Options,
        progress_reporter: ProgressReporter,
        rng: Rng,
    ) -> Self {
        Self::new_with_progress(problem, options, progress_reporter, rng)
    }

    fn new_with_progress(
        problem: &'a PreparedProblem,
        options: &'a Options,
        progress_reporter: ProgressReporter,
        rng: Rng,
    ) -> Self {
        Self {
            problem,
            options,
            current_solution: Some(Scheduling::new()),
            has_solution: false,
            progress_reporter,
            rng,
            current_depth: 0,
            max_depth_reached: 0,
        }
    }

    /// Advances the search to the next feasible scheduling.
    ///
    /// # Panics
    ///
    /// Panics if `critical_set_timeout_seconds` cannot be converted to `u64`.
    pub fn next_scheduling(&mut self, deadline: Option<SystemTime>) -> bool {
        let input_data = &self.problem.input_data;
        self.current_depth = 0;
        self.max_depth_reached = 0;
        let mut preference_limit = if self.rng.next_in_range(0, Self::PREF_RELAXATION) == 0 {
            input_data.max_preference
        } else {
            self.problem.critical_set_analysis.preference_bound()
        };
        status::trace(&format!(
            "Starting scheduling search with preference limit {preference_limit}."
        ));

        let slots = loop {
            let cs_sets = self
                .problem
                .critical_set_analysis
                .for_preference(preference_limit);
            let time_limit = if preference_limit == input_data.max_preference {
                time_never()
            } else {
                time_now()
                    + Duration::from_secs(
                        u64::try_from(self.options.critical_set_timeout_seconds)
                            .expect("critical set timeout must be non-negative"),
                    )
            };
            let time_limit = deadline.map_or(time_limit, |deadline| min(time_limit, deadline));

            let slots = self.solve_scheduling(cs_sets, time_limit);
            if !slots.is_empty() {
                status::trace(&format!(
                    "Scheduling search succeeded with preference limit {preference_limit}."
                ));
                break slots;
            }

            if preference_limit == input_data.max_preference {
                status::debug(
                    "Scheduling search exhausted all preference limits without a solution.",
                );
                self.has_solution = false;
                self.current_solution = None;
                return false;
            }

            status::trace(&format!(
                "No scheduling found at preference limit {preference_limit}; relaxing limit."
            ));
            preference_limit = input_data.preference_after(preference_limit);
        };

        let mut scheduling = vec![None; input_data.choices.len()];
        for (slot_index, choices) in slots.iter().enumerate() {
            for &choice in choices {
                scheduling[choice] = Some(slot_index);
            }
        }

        self.has_solution = true;
        self.current_depth = input_data.choices.len();
        self.publish_depth_progress();
        self.current_solution = Some(Scheduling::with_data(input_data, scheduling));
        true
    }

    /// Returns the most recently generated scheduling, if any.
    #[must_use]
    pub fn scheduling(&self) -> Option<Scheduling> {
        self.current_solution.clone()
    }

    fn satisfies_critical_sets(
        &self,
        decisions: &[SchedulingDecision],
        critical_sets: &[CriticalSet],
    ) -> bool {
        let slot_count = self.problem.input_data.slots.len();
        for set in critical_sets {
            let mut covered_slots = vec![false; slot_count];
            let mut covered_count = 0_usize;
            let mut missing = 0_usize;

            for &element in &set.data {
                match decisions[element] {
                    SchedulingDecision::Undecided => missing += 1,
                    SchedulingDecision::Slot(slot) => {
                        if !covered_slots[slot] {
                            covered_slots[slot] = true;
                            covered_count += 1;
                        }
                    }
                    SchedulingDecision::Omitted => {}
                }
            }

            if covered_count + missing < slot_count {
                return false;
            }
        }

        true
    }

    fn satisfies_scheduling_constraints(
        &self,
        choice: usize,
        slot: Option<usize>,
        state: &SchedulingSearchState,
    ) -> bool {
        let input_data = &self.problem.input_data;
        for constraint in input_data.scheduling_constraints_for(choice) {
            match constraint.kind {
                ConstraintType::ChoiceIsInSlot => {
                    if slot != Some(constraint.slot()) {
                        return false;
                    }
                }
                ConstraintType::ChoiceIsNotInSlot => {
                    if slot == Some(constraint.slot()) {
                        return false;
                    }
                }
                ConstraintType::ChoicesAreInSameSlot => {
                    let other = if constraint.left == choice {
                        constraint.other_choice()
                    } else {
                        constraint.left
                    };
                    let other_slot = state.decisions[other].slot();
                    if state.decisions[other] != SchedulingDecision::Undecided && other_slot != slot
                    {
                        return false;
                    }
                }
                ConstraintType::ChoicesAreNotInSameSlot => {
                    let other = if constraint.left == choice {
                        constraint.other_choice()
                    } else {
                        constraint.left
                    };
                    let other_slot = state.decisions[other].slot();
                    if state.decisions[other] != SchedulingDecision::Undecided
                        && other_slot == slot
                        && slot.is_some()
                    {
                        return false;
                    }
                }
                ConstraintType::ChoicesHaveOffset => {
                    let other = if constraint.left == choice {
                        constraint.other_choice()
                    } else {
                        constraint.left
                    };
                    let offset = if other == constraint.left {
                        -constraint.offset()
                    } else {
                        constraint.offset()
                    };
                    if state.decisions[other] != SchedulingDecision::Undecided {
                        let other_slot = state.decisions[other].slot();
                        if other_slot.is_some() != slot.is_some() {
                            return false;
                        }
                        let (Some(other_slot), Some(slot)) = (other_slot, slot) else {
                            continue;
                        };
                        if i32::try_from(other_slot).expect("slot index must fit in i32")
                            - i32::try_from(slot).expect("slot index must fit in i32")
                            != offset
                        {
                            return false;
                        }

                        let min_slot = max(0, -offset);
                        let max_slot = min(
                            i32::try_from(input_data.slots.len())
                                .expect("slot count must fit in i32"),
                            i32::try_from(input_data.slots.len())
                                .expect("slot count must fit in i32")
                                - offset,
                        );
                        let slot = i32::try_from(slot).expect("slot index must fit in i32");
                        if slot < min_slot || slot >= max_slot {
                            return false;
                        }
                    }
                }
                ConstraintType::SlotHasLimitedSize => {
                    if slot != Some(constraint.left) {
                        continue;
                    }

                    if matches!(
                        constraint.slot_size_limit_op(),
                        crate::SlotSizeLimitOp::Gt
                            | crate::SlotSizeLimitOp::Geq
                            | crate::SlotSizeLimitOp::Neq
                    ) {
                        continue;
                    }

                    let mut limit = i32::try_from(constraint.limit())
                        .expect("limit must fit in i32")
                        - i32::from(matches!(
                            constraint.slot_size_limit_op(),
                            crate::SlotSizeLimitOp::Lt
                        ));
                    limit -= i32::try_from(slot.map_or(0, |slot| state.slot_choice_count[slot]))
                        .expect("slot choice count must fit in i32");
                    if limit < 1 {
                        return false;
                    }
                }
                _ => panic!("Unknown scheduling type {:?}.", constraint.kind),
            }
        }

        if state
            .decisions
            .iter()
            .filter(|&&decision| decision != SchedulingDecision::Undecided)
            .count()
            + 1
            == input_data.choices.len()
        {
            self.check_slot_size_constraints(slot, &state.slot_choice_count)
        } else {
            true
        }
    }

    fn check_slot_size_constraints(&self, slot: Option<usize>, slot_choice_count: &[u32]) -> bool {
        let input_data = &self.problem.input_data;

        for constraint in &input_data.scheduling_constraints {
            if constraint.kind != ConstraintType::SlotHasLimitedSize {
                continue;
            }
            let count =
                slot_choice_count[constraint.left] + u32::from(slot == Some(constraint.left));
            let valid = match constraint.slot_size_limit_op() {
                crate::SlotSizeLimitOp::Eq => count == constraint.limit(),
                crate::SlotSizeLimitOp::Neq => count != constraint.limit(),
                crate::SlotSizeLimitOp::Lt => count < constraint.limit(),
                crate::SlotSizeLimitOp::Leq => count <= constraint.limit(),
                crate::SlotSizeLimitOp::Gt => count > constraint.limit(),
                crate::SlotSizeLimitOp::Geq => count >= constraint.limit(),
            };
            if !valid {
                return false;
            }
        }

        true
    }

    fn has_impossibilities(&self, state: &SchedulingSearchState, available_max_push: u32) -> bool {
        let input_data = &self.problem.input_data;
        for slot in 0..input_data.slots.len() {
            let sum = available_max_push + state.slot_max[slot];
            if sum
                < u32::try_from(input_data.choosers.len()).expect("chooser count must fit in u32")
            {
                return true;
            }
        }
        false
    }

    fn calculate_critical_slots(
        &self,
        state: &SchedulingSearchState,
        available_max_push: u32,
        choice: usize,
    ) -> Vec<Option<usize>> {
        let input_data = &self.problem.input_data;
        let mut critical_slots = Vec::new();
        for slot in 0..input_data.slots.len() {
            let sum = available_max_push - input_data.choices[choice].max + state.slot_max[slot];
            if sum
                < u32::try_from(input_data.choosers.len()).expect("chooser count must fit in u32")
            {
                critical_slots.push(Some(slot));
            }
        }
        critical_slots
    }

    fn slot_order_heuristic_score(
        &self,
        state: &SchedulingSearchState,
        slot: Option<usize>,
    ) -> u32 {
        slot.map_or(0, |slot| state.slot_max[slot])
    }

    fn calculate_feasible_slots(
        &mut self,
        state: &SchedulingSearchState,
        low_priority_slot: &[bool],
        choice: usize,
    ) -> Vec<Option<usize>> {
        let input_data = &self.problem.input_data;
        let mut normal_slots = Vec::new();
        let mut low_slots = Vec::new();

        if input_data.choices[choice].is_optional
            && self.satisfies_scheduling_constraints(choice, None, state)
        {
            low_slots.push(None);
        }

        for (slot, &is_low_priority) in low_priority_slot
            .iter()
            .enumerate()
            .take(input_data.slots.len())
        {
            let sum = input_data.choices[choice].min + state.slot_min[slot];

            if sum
                > u32::try_from(input_data.choosers.len()).expect("chooser count must fit in u32")
                || !self.satisfies_scheduling_constraints(choice, Some(slot), state)
            {
                continue;
            }

            if is_low_priority {
                low_slots.push(Some(slot));
            } else {
                normal_slots.push(Some(slot));
            }
        }

        normal_slots.sort_by_key(|&slot| (self.slot_order_heuristic_score(state, slot), slot));
        riffle_shuffle(normal_slots, low_slots, &mut self.rng)
    }

    fn get_choice_scramble(&mut self) -> Vec<usize> {
        let input_data = &self.problem.input_data;
        let mut scramble = (0..input_data.choices.len()).collect::<Vec<_>>();
        self.rng.shuffle(&mut scramble);
        scramble
            .sort_by_key(|&choice| Reverse(input_data.scheduling_constraints_for(choice).len()));
        scramble
    }

    fn get_low_priority_slots(&self) -> Vec<bool> {
        vec![false; self.problem.input_data.slots.len()]
    }

    fn convert_decisions(&self, decisions: &[SchedulingDecision]) -> Vec<Vec<usize>> {
        let mut result = vec![Vec::new(); self.problem.input_data.slots.len()];
        for (choice, &decision) in decisions.iter().enumerate() {
            if let SchedulingDecision::Slot(slot) = decision {
                result[slot].push(choice);
            }
        }
        result
    }

    fn apply_decision(
        &self,
        state: &mut SchedulingSearchState,
        choice: usize,
        slot: Option<usize>,
    ) {
        let previous = state.decisions[choice];
        if let SchedulingDecision::Slot(previous_slot) = previous {
            let data = &self.problem.input_data.choices[choice];
            state.slot_min[previous_slot] -= data.min;
            state.slot_max[previous_slot] -= data.max;
            state.slot_choice_count[previous_slot] -= 1;
        }

        state.decisions[choice] = match slot {
            Some(slot) => {
                let data = &self.problem.input_data.choices[choice];
                state.slot_min[slot] += data.min;
                state.slot_max[slot] += data.max;
                state.slot_choice_count[slot] += 1;
                SchedulingDecision::Slot(slot)
            }
            None => SchedulingDecision::Omitted,
        };
    }

    fn clear_decision(&self, state: &mut SchedulingSearchState, choice: usize) {
        if let SchedulingDecision::Slot(slot) = state.decisions[choice] {
            let data = &self.problem.input_data.choices[choice];
            state.slot_min[slot] -= data.min;
            state.slot_max[slot] -= data.max;
            state.slot_choice_count[slot] -= 1;
        }
        state.decisions[choice] = SchedulingDecision::Undecided;
    }

    fn solve_scheduling(
        &mut self,
        critical_sets: &[CriticalSet],
        time_limit: SystemTime,
    ) -> Vec<Vec<usize>> {
        let input_data = &self.problem.input_data;
        let choice_scramble = self.get_choice_scramble();
        let low_priority_slots = self.get_low_priority_slots();
        let mut state =
            SchedulingSearchState::new(input_data.choices.len(), input_data.slots.len());
        let mut backtracking = Vec::<Vec<Option<usize>>>::new();
        let mut depth = 0_usize;
        let mut available_max_suffix = vec![0_u32; choice_scramble.len() + 1];
        for depth_index in (0..choice_scramble.len()).rev() {
            available_max_suffix[depth_index] = available_max_suffix[depth_index + 1]
                + input_data.choices[choice_scramble[depth_index]].max;
        }

        while depth < choice_scramble.len() {
            self.current_depth = depth;
            self.max_depth_reached = self.max_depth_reached.max(depth);
            self.publish_depth_progress();
            if time_now() > time_limit {
                status::debug("Scheduling search hit its current time limit.");
                return Vec::new();
            }

            let choice = choice_scramble[depth];
            if backtracking.len() <= depth {
                let available_max_push = available_max_suffix[depth];
                let candidates = if self.has_impossibilities(&state, available_max_push)
                    || !self.satisfies_critical_sets(&state.decisions, critical_sets)
                {
                    Vec::new()
                } else {
                    let critical_slots =
                        self.calculate_critical_slots(&state, available_max_push, choice);
                    match critical_slots.len().cmp(&1) {
                        std::cmp::Ordering::Equal => {
                            if self.satisfies_scheduling_constraints(
                                choice,
                                critical_slots[0],
                                &state,
                            ) {
                                critical_slots
                            } else {
                                Vec::new()
                            }
                        }
                        std::cmp::Ordering::Greater => Vec::new(),
                        std::cmp::Ordering::Less => {
                            self.calculate_feasible_slots(&state, &low_priority_slots, choice)
                        }
                    }
                };
                backtracking.push(candidates.into_iter().rev().collect());
            }

            if backtracking[depth].is_empty() {
                if depth == 0 {
                    return Vec::new();
                }

                backtracking.pop();
                self.clear_decision(&mut state, choice_scramble[depth - 1]);
                depth -= 1;
                self.current_depth = depth;
                self.publish_depth_progress();
                continue;
            }

            let next_slot = backtracking[depth]
                .pop()
                .expect("backtracking bucket should contain a candidate");
            self.apply_decision(&mut state, choice, next_slot);
            depth += 1;
            self.current_depth = depth;
            self.max_depth_reached = self.max_depth_reached.max(depth);
            self.publish_depth_progress();
        }

        let result = self.convert_decisions(&state.decisions);
        let mut scheduling = vec![None; input_data.choices.len()];
        for (slot_index, choices) in result.iter().enumerate() {
            for &choice in choices {
                scheduling[choice] = Some(slot_index);
            }
        }
        if !Scheduling::with_data(input_data, scheduling).is_feasible(input_data) {
            return Vec::new();
        }
        result
    }

    fn publish_depth_progress(&self) {
        self.progress_reporter
            .publish_depth(u16::try_from(self.current_depth).map_or(f32::INFINITY, f32::from));
    }
}
