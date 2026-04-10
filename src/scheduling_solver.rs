use std::cmp::{Reverse, max, min};
use std::collections::BTreeMap;
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

    fn calculate_available_max_push(&self, choice_scramble: &[usize], depth: usize) -> u32 {
        let input_data = &self.problem.input_data;
        choice_scramble[depth..]
            .iter()
            .map(|&choice| input_data.choices[choice].max)
            .sum()
    }

    fn satisfies_critical_sets(
        &self,
        decisions: &BTreeMap<usize, Option<usize>>,
        critical_sets: &[CriticalSet],
    ) -> bool {
        let slot_count = self.problem.input_data.slots.len();
        for set in critical_sets {
            let mut covered_slots = std::collections::BTreeSet::new();
            let mut missing = 0_usize;

            for &element in &set.data {
                match decisions.get(&element).copied() {
                    None => missing += 1,
                    Some(Some(slot)) => {
                        covered_slots.insert(slot);
                    }
                    Some(None) => {}
                }
            }

            if covered_slots.len() + missing < slot_count {
                return false;
            }
        }

        true
    }

    fn satisfies_scheduling_constraints(
        &self,
        choice: usize,
        slot: Option<usize>,
        decisions: &BTreeMap<usize, Option<usize>>,
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
                    if let Some(&other_slot) = decisions.get(&other)
                        && other_slot != slot
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
                    if let Some(&other_slot) = decisions.get(&other)
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
                    if let Some(&other_slot) = decisions.get(&other) {
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
                    for &decision_slot in decisions.values() {
                        if decision_slot == slot {
                            limit -= 1;
                        }
                    }
                    if limit < 1 {
                        return false;
                    }
                }
                _ => panic!("Unknown scheduling type {:?}.", constraint.kind),
            }
        }

        if decisions.len() + 1 == input_data.choices.len() {
            self.check_slot_size_constraints(choice, slot, decisions)
        } else {
            true
        }
    }

    fn check_slot_size_constraints(
        &self,
        choice: usize,
        slot: Option<usize>,
        decisions: &BTreeMap<usize, Option<usize>>,
    ) -> bool {
        let input_data = &self.problem.input_data;
        let Some(slot) = slot else {
            return true;
        };

        let mut slot_sizes: Option<Vec<u32>> = None;

        for constraint in &input_data.scheduling_constraints {
            if constraint.kind != ConstraintType::SlotHasLimitedSize {
                continue;
            }

            let sizes = slot_sizes.get_or_insert_with(|| {
                let mut sizes = vec![0_u32; input_data.slots.len()];
                sizes[slot] += 1;
                for (&decision_choice, &decision_slot) in decisions {
                    if decision_choice == choice {
                        continue;
                    }
                    if let Some(decision_slot) = decision_slot {
                        sizes[decision_slot] += 1;
                    }
                }
                sizes
            });

            let count = sizes[constraint.left];
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

    fn has_impossibilities(
        &self,
        decisions: &BTreeMap<usize, Option<usize>>,
        available_max_push: u32,
    ) -> bool {
        let input_data = &self.problem.input_data;
        for slot in 0..input_data.slots.len() {
            let mut sum = available_max_push;
            for (&choice, &decision_slot) in decisions {
                if decision_slot == Some(slot) {
                    sum += input_data.choices[choice].max;
                }
            }
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
        decisions: &BTreeMap<usize, Option<usize>>,
        available_max_push: u32,
        choice: usize,
    ) -> Vec<Option<usize>> {
        let input_data = &self.problem.input_data;
        let mut critical_slots = Vec::new();
        for slot in 0..input_data.slots.len() {
            let mut sum = available_max_push - input_data.choices[choice].max;
            for (&other_choice, &decision_slot) in decisions {
                if other_choice != choice && decision_slot == Some(slot) {
                    sum += input_data.choices[other_choice].max;
                }
            }
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
        decisions: &BTreeMap<usize, Option<usize>>,
        slot: Option<usize>,
    ) -> u32 {
        let input_data = &self.problem.input_data;
        decisions
            .iter()
            .filter(|(_, decision_slot)| **decision_slot == slot)
            .map(|(&choice, _)| input_data.choices[choice].max)
            .sum()
    }

    fn calculate_feasible_slots(
        &mut self,
        decisions: &BTreeMap<usize, Option<usize>>,
        low_priority_slot: &[bool],
        choice: usize,
    ) -> Vec<Option<usize>> {
        let input_data = &self.problem.input_data;
        let mut normal_slots = Vec::new();
        let mut low_slots = Vec::new();

        if input_data.choices[choice].is_optional
            && self.satisfies_scheduling_constraints(choice, None, decisions)
        {
            low_slots.push(None);
        }

        for (slot, &is_low_priority) in low_priority_slot
            .iter()
            .enumerate()
            .take(input_data.slots.len())
        {
            let mut sum = input_data.choices[choice].min;
            for (&other_choice, &decision_slot) in decisions {
                if decision_slot == Some(slot) {
                    sum += input_data.choices[other_choice].min;
                }
            }

            if sum
                > u32::try_from(input_data.choosers.len()).expect("chooser count must fit in u32")
                || !self.satisfies_scheduling_constraints(choice, Some(slot), decisions)
            {
                continue;
            }

            if is_low_priority {
                low_slots.push(Some(slot));
            } else {
                normal_slots.push(Some(slot));
            }
        }

        normal_slots.sort_by_key(|&slot| (self.slot_order_heuristic_score(decisions, slot), slot));
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

    fn convert_decisions(&self, decisions: &BTreeMap<usize, Option<usize>>) -> Vec<Vec<usize>> {
        let mut result = vec![Vec::new(); self.problem.input_data.slots.len()];
        for (&choice, &slot) in decisions {
            if let Some(slot) = slot {
                result[slot].push(choice);
            }
        }
        result
    }

    fn solve_scheduling(
        &mut self,
        critical_sets: &[CriticalSet],
        time_limit: SystemTime,
    ) -> Vec<Vec<usize>> {
        let input_data = &self.problem.input_data;
        let choice_scramble = self.get_choice_scramble();
        let low_priority_slots = self.get_low_priority_slots();
        let mut decisions = BTreeMap::<usize, Option<usize>>::new();
        let mut backtracking = Vec::<Vec<Option<usize>>>::new();
        let mut depth = 0_usize;

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
                let available_max_push = self.calculate_available_max_push(&choice_scramble, depth);
                let candidates = if self.has_impossibilities(&decisions, available_max_push)
                    || !self.satisfies_critical_sets(&decisions, critical_sets)
                {
                    Vec::new()
                } else {
                    let critical_slots =
                        self.calculate_critical_slots(&decisions, available_max_push, choice);
                    match critical_slots.len().cmp(&1) {
                        std::cmp::Ordering::Equal => {
                            if self.satisfies_scheduling_constraints(
                                choice,
                                critical_slots[0],
                                &decisions,
                            ) {
                                critical_slots
                            } else {
                                Vec::new()
                            }
                        }
                        std::cmp::Ordering::Greater => Vec::new(),
                        std::cmp::Ordering::Less => {
                            self.calculate_feasible_slots(&decisions, &low_priority_slots, choice)
                        }
                    }
                };
                backtracking.push(candidates);
            }

            if backtracking[depth].is_empty() {
                if depth == 0 {
                    return Vec::new();
                }

                backtracking.pop();
                decisions.remove(&choice_scramble[depth - 1]);
                depth -= 1;
                self.current_depth = depth;
                self.publish_depth_progress();
                continue;
            }

            let next_slot = backtracking[depth].remove(0);
            decisions.insert(choice, next_slot);
            depth += 1;
            self.current_depth = depth;
            self.max_depth_reached = self.max_depth_reached.max(depth);
            self.publish_depth_progress();
        }

        let result = self.convert_decisions(&decisions);
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
