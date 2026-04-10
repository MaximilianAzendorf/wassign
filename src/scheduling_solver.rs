use std::cmp::{Reverse, max, min};
use std::collections::BTreeMap;
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime};

use crate::shotgun_solver::ShotgunSolverProgress;
use crate::status;
use crate::util::{riffle_shuffle, time_never, time_now};
use crate::{
    Constraint, ConstraintType, CriticalSet, CriticalSetAnalysis, InputData, Options, Rng,
    Scheduling,
};

/// Enumerates feasible schedulings for a given input.
#[derive(Debug)]
pub struct SchedulingSolver {
    input_data: Arc<InputData>,
    cs_analysis: Arc<CriticalSetAnalysis>,
    current_solution: Option<Arc<Scheduling>>,
    has_solution: bool,
    options: Arc<Options>,
    rng: Rng,
    progress_sink: Option<Arc<Mutex<ShotgunSolverProgress>>>,
    pub(crate) current_depth: usize,
    pub(crate) max_depth_reached: usize,
}

impl SchedulingSolver {
    pub(crate) const PREF_RELAXATION: i32 = 10;

    /// Creates a new scheduling solver.
    #[must_use]
    pub fn new(
        input_data: Arc<InputData>,
        cs_analysis: Arc<CriticalSetAnalysis>,
        options: Arc<Options>,
    ) -> Self {
        Self::new_with_rng(input_data, cs_analysis, options, Rng::from_global_seed())
    }

    pub(crate) fn new_with_rng(
        input_data: Arc<InputData>,
        cs_analysis: Arc<CriticalSetAnalysis>,
        options: Arc<Options>,
        rng: Rng,
    ) -> Self {
        Self {
            current_solution: Some(Arc::new(Scheduling::new(input_data.clone()))),
            has_solution: false,
            input_data,
            cs_analysis,
            options,
            rng,
            progress_sink: None,
            current_depth: 0,
            max_depth_reached: 0,
        }
    }

    pub(crate) fn set_progress_sink(&mut self, progress_sink: Arc<Mutex<ShotgunSolverProgress>>) {
        self.progress_sink = Some(progress_sink);
    }

    /// Advances the search to the next feasible scheduling.
    ///
    /// Returns `false` when no further scheduling exists.
    ///
    /// # Panics
    ///
    /// Panics if an internal slot index does not fit into `i32`.
    pub fn next_scheduling(&mut self, deadline: Option<SystemTime>) -> bool {
        self.current_depth = 0;
        self.max_depth_reached = 0;
        let mut preference_limit = if self.rng.next_in_range(0, Self::PREF_RELAXATION) == 0 {
            self.input_data.max_preference
        } else {
            self.cs_analysis.preference_bound()
        };
        status::trace(&format!(
            "Starting scheduling search with preference limit {preference_limit}."
        ));

        let slots = loop {
            let cs_sets = self.cs_analysis.for_preference(preference_limit);
            let time_limit = if preference_limit == self.input_data.max_preference {
                time_never()
            } else {
                time_now()
                    + Duration::from_secs(
                        u64::try_from(self.options.critical_set_timeout_seconds)
                            .expect("critical set timeout must be non-negative"),
                    )
            };
            let time_limit = deadline.map_or(time_limit, |deadline| min(time_limit, deadline));

            let slots = self.solve_scheduling(cs_sets.as_ref(), time_limit);
            if !slots.is_empty() {
                status::trace(&format!(
                    "Scheduling search succeeded with preference limit {preference_limit}."
                ));
                break slots;
            }

            if preference_limit == self.input_data.max_preference {
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
            preference_limit = self.input_data.preference_after(preference_limit);
        };

        let mut scheduling = vec![Scheduling::NOT_SCHEDULED; self.input_data.choices.len()];
        for (slot_index, choices) in slots.iter().enumerate() {
            for &choice in choices {
                scheduling[choice] = i32::try_from(slot_index).expect("slot index must fit in i32");
            }
        }

        self.has_solution = true;
        self.current_depth = self.input_data.choices.len();
        self.publish_depth_progress();
        self.current_solution = Some(Arc::new(Scheduling::with_data(
            self.input_data.clone(),
            scheduling,
        )));
        status::trace("Produced a new feasible scheduling.");
        true
    }

    /// Returns the most recently generated scheduling, if any.
    #[must_use]
    pub fn scheduling(&self) -> Option<Arc<Scheduling>> {
        self.current_solution.clone()
    }

    fn calculate_available_max_push(&self, choice_scramble: &[usize], depth: usize) -> i32 {
        choice_scramble[depth..]
            .iter()
            .map(|&choice| self.input_data.choices[choice].max)
            .sum()
    }

    fn satisfies_critical_sets(
        &self,
        decisions: &BTreeMap<usize, i32>,
        critical_sets: &[CriticalSet],
    ) -> bool {
        let slot_count = self.input_data.slots.len();
        for set in critical_sets {
            let mut covered_slots = std::collections::BTreeSet::new();
            let mut missing = 0_usize;

            for &element in &set.data {
                match decisions.get(&element).copied() {
                    None => missing += 1,
                    Some(slot) if slot != Scheduling::NOT_SCHEDULED => {
                        covered_slots.insert(slot);
                    }
                    Some(_) => {}
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
        slot: i32,
        decisions: &BTreeMap<usize, i32>,
    ) -> bool {
        for constraint in self.input_data.scheduling_constraints_for(choice) {
            match constraint.kind {
                ConstraintType::ChoiceIsInSlot => {
                    if slot != constraint.right {
                        return false;
                    }
                }
                ConstraintType::ChoiceIsNotInSlot => {
                    if slot == constraint.right {
                        return false;
                    }
                }
                ConstraintType::ChoicesAreInSameSlot => {
                    let other = if constraint.left == choice {
                        usize::try_from(constraint.right)
                            .expect("choice index must be non-negative")
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
                        usize::try_from(constraint.right)
                            .expect("choice index must be non-negative")
                    } else {
                        constraint.left
                    };
                    if let Some(&other_slot) = decisions.get(&other)
                        && other_slot == slot
                        && slot != Scheduling::NOT_SCHEDULED
                    {
                        return false;
                    }
                }
                ConstraintType::ChoicesHaveOffset => {
                    let other = if constraint.left == choice {
                        usize::try_from(constraint.right)
                            .expect("choice index must be non-negative")
                    } else {
                        constraint.left
                    };
                    let offset = if other == constraint.left {
                        -constraint.extra
                    } else {
                        constraint.extra
                    };
                    if let Some(&other_slot) = decisions.get(&other) {
                        if (other_slot == Scheduling::NOT_SCHEDULED)
                            != (slot == Scheduling::NOT_SCHEDULED)
                        {
                            return false;
                        }
                        if slot == Scheduling::NOT_SCHEDULED {
                            continue;
                        }
                        if other_slot - slot != offset {
                            return false;
                        }

                        let min_slot = max(0, -offset);
                        let max_slot = min(
                            i32::try_from(self.input_data.slots.len())
                                .expect("slot count must fit in i32"),
                            i32::try_from(self.input_data.slots.len())
                                .expect("slot count must fit in i32")
                                - offset,
                        );
                        if slot < min_slot || slot >= max_slot {
                            return false;
                        }
                    }
                }
                ConstraintType::SlotHasLimitedSize => {
                    if i32::try_from(constraint.left).expect("slot index must fit in i32") != slot {
                        continue;
                    }

                    if matches!(constraint.extra, 2 | 3 | -1) {
                        continue;
                    }

                    let mut limit = constraint.right - i32::from(constraint.extra == -3);
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

        if decisions.len() + 1 == self.input_data.choices.len() {
            self.check_slot_size_constraints(choice, slot, decisions)
        } else {
            true
        }
    }

    fn check_slot_size_constraints(
        &self,
        choice: usize,
        slot: i32,
        decisions: &BTreeMap<usize, i32>,
    ) -> bool {
        if slot < 0 {
            return true;
        }

        let mut slot_sizes: Option<Vec<i32>> = None;

        for constraint in &self.input_data.scheduling_constraints {
            if constraint.kind != ConstraintType::SlotHasLimitedSize {
                continue;
            }

            let sizes = slot_sizes.get_or_insert_with(|| {
                let mut sizes = vec![0_i32; self.input_data.slots.len()];
                sizes[usize::try_from(slot).expect("slot id must be non-negative")] += 1;
                for (&decision_choice, &decision_slot) in decisions {
                    if decision_choice == choice || decision_slot == Scheduling::NOT_SCHEDULED {
                        continue;
                    }
                    sizes[usize::try_from(decision_slot).expect("slot id must be non-negative")] +=
                        1;
                }
                sizes
            });

            let count = sizes[constraint.left];
            let valid = match constraint.extra {
                1 => count == constraint.right,
                -1 => count != constraint.right,
                -3 => count < constraint.right,
                -2 => count <= constraint.right,
                2 => count > constraint.right,
                3 => count >= constraint.right,
                extra => panic!("Unknown slot size limit operator {extra}."),
            };
            if !valid {
                return false;
            }
        }

        true
    }

    fn has_impossibilities(
        &self,
        decisions: &BTreeMap<usize, i32>,
        available_max_push: i32,
    ) -> bool {
        for slot in 0..self.input_data.slots.len() {
            let slot_i32 = i32::try_from(slot).expect("slot index must fit in i32");
            let mut sum = available_max_push;
            for (&choice, &decision_slot) in decisions {
                if decision_slot == slot_i32 {
                    sum += self.input_data.choices[choice].max;
                }
            }
            if sum
                < i32::try_from(self.input_data.choosers.len())
                    .expect("chooser count must fit in i32")
            {
                return true;
            }
        }
        false
    }

    fn calculate_critical_slots(
        &self,
        decisions: &BTreeMap<usize, i32>,
        available_max_push: i32,
        choice: usize,
    ) -> Vec<i32> {
        let mut critical_slots = Vec::new();
        for slot in 0..self.input_data.slots.len() {
            let slot_i32 = i32::try_from(slot).expect("slot index must fit in i32");
            let mut sum = available_max_push - self.input_data.choices[choice].max;
            for (&other_choice, &decision_slot) in decisions {
                if other_choice != choice && decision_slot == slot_i32 {
                    sum += self.input_data.choices[other_choice].max;
                }
            }
            if sum
                < i32::try_from(self.input_data.choosers.len())
                    .expect("chooser count must fit in i32")
            {
                critical_slots.push(slot_i32);
            }
        }
        critical_slots
    }

    fn slot_order_heuristic_score(&self, decisions: &BTreeMap<usize, i32>, slot: i32) -> i32 {
        decisions
            .iter()
            .filter(|(_, decision_slot)| **decision_slot == slot)
            .map(|(&choice, _)| self.input_data.choices[choice].max)
            .sum()
    }

    fn calculate_feasible_slots(
        &mut self,
        decisions: &BTreeMap<usize, i32>,
        low_priority_slot: &[bool],
        choice: usize,
    ) -> Vec<i32> {
        let mut normal_slots = Vec::new();
        let mut low_slots = Vec::new();

        if self.input_data.choices[choice].is_optional
            && self.satisfies_scheduling_constraints(choice, Scheduling::NOT_SCHEDULED, decisions)
        {
            low_slots.push(Scheduling::NOT_SCHEDULED);
        }

        for (slot, &is_low_priority) in low_priority_slot
            .iter()
            .enumerate()
            .take(self.input_data.slots.len())
        {
            let slot_i32 = i32::try_from(slot).expect("slot index must fit in i32");
            let mut sum = self.input_data.choices[choice].min;
            for (&other_choice, &decision_slot) in decisions {
                if decision_slot == slot_i32 {
                    sum += self.input_data.choices[other_choice].min;
                }
            }

            if sum
                > i32::try_from(self.input_data.choosers.len())
                    .expect("chooser count must fit in i32")
                || !self.satisfies_scheduling_constraints(choice, slot_i32, decisions)
            {
                continue;
            }

            if is_low_priority {
                low_slots.push(slot_i32);
            } else {
                normal_slots.push(slot_i32);
            }
        }

        normal_slots.sort_by_key(|&slot| (self.slot_order_heuristic_score(decisions, slot), slot));
        riffle_shuffle(normal_slots, low_slots, &mut self.rng)
    }

    fn get_choice_scramble(&mut self) -> Vec<usize> {
        let mut scramble = (0..self.input_data.choices.len()).collect::<Vec<_>>();
        self.rng.shuffle(&mut scramble);
        scramble.sort_by_key(|&choice| Reverse(self.scheduling_constraints(choice).len()));
        scramble
    }

    fn get_low_priority_slots(&self) -> Vec<bool> {
        vec![false; self.input_data.slots.len()]
    }

    fn convert_decisions(&self, decisions: &BTreeMap<usize, i32>) -> Vec<Vec<usize>> {
        let mut result = vec![Vec::new(); self.input_data.slots.len()];
        for (&choice, &slot) in decisions {
            if slot >= 0 {
                result[usize::try_from(slot).expect("slot id must be non-negative")].push(choice);
            }
        }
        result
    }

    fn solve_scheduling(
        &mut self,
        critical_sets: &[CriticalSet],
        time_limit: std::time::SystemTime,
    ) -> Vec<Vec<usize>> {
        let choice_scramble = self.get_choice_scramble();
        let low_priority_slots = self.get_low_priority_slots();
        let mut decisions = BTreeMap::<usize, i32>::new();
        let mut backtracking = Vec::<Vec<i32>>::new();
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

        self.convert_decisions(&decisions)
    }

    fn publish_depth_progress(&self) {
        let Some(progress_sink) = &self.progress_sink else {
            return;
        };

        progress_sink
            .lock()
            .expect("progress mutex poisoned")
            .sched_depth = u16::try_from(self.current_depth).map_or(f32::INFINITY, f32::from);
    }

    fn scheduling_constraints(&self, choice: usize) -> &[Constraint] {
        self.input_data
            .choice_constraint_map
            .get(&choice)
            .map_or(&[], Vec::as_slice)
    }
}
