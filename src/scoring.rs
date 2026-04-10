use std::sync::Arc;

use crate::{Assignment, InputData, Options, Scheduling, Score, Solution};

/// Evaluates complete solutions according to feasibility and preference quality.
#[derive(Debug, Clone)]
pub struct Scoring {
    input_data: Arc<InputData>,
    options: Arc<Options>,
    scaling: f32,
}

impl Scoring {
    /// Creates a new scorer for the given input and options.
    #[must_use]
    pub fn new(input_data: Arc<InputData>, options: Arc<Options>) -> Self {
        let scaling = (input_data.max_preference as f32).powf(options.preference_exponent as f32);
        Self {
            input_data,
            options,
            scaling,
        }
    }

    pub(crate) fn is_feasible(&self, solution: &Solution) -> bool {
        let scheduling = solution
            .scheduling()
            .expect("feasibility requires a scheduling");
        let assignment = solution
            .assignment()
            .expect("feasibility requires an assignment");

        let mut part_counts = vec![0_i32; self.input_data.choices.len()];
        let mut is_in_slot =
            vec![vec![false; self.input_data.slots.len()]; self.input_data.choosers.len()];
        let slots = (0..self.input_data.choices.len())
            .map(|choice| scheduling.slot_of(choice))
            .collect::<Vec<_>>();

        if !Self::satisfies_constraints_scheduling(scheduling)
            || !Self::satisfies_constraints_assignment(scheduling, assignment)
        {
            return false;
        }

        for (chooser, slots_in) in is_in_slot
            .iter_mut()
            .enumerate()
            .take(self.input_data.choosers.len())
        {
            for slot in 0..self.input_data.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                let scheduled_slot =
                    usize::try_from(slots[choice]).expect("choice must be scheduled");
                if slots_in[scheduled_slot] {
                    return false;
                }
                slots_in[scheduled_slot] = true;
                part_counts[choice] += 1;
            }
        }

        for (choice_index, &count) in part_counts.iter().enumerate() {
            if scheduling.slot_of(choice_index) == Scheduling::NOT_SCHEDULED {
                continue;
            }
            let choice = &self.input_data.choices[choice_index];
            if count < choice.min || count > choice.max {
                return false;
            }
        }

        true
    }

    /// Evaluates a solution and returns its score.
    #[must_use]
    pub fn evaluate(&self, solution: &Solution) -> Score {
        if solution.is_invalid() {
            return Score {
                major: f32::INFINITY,
                minor: f32::INFINITY,
            };
        }

        let major = if self.options.greedy {
            f32::NAN
        } else {
            self.evaluate_major(solution) as f32
        };
        let minor = self.evaluate_minor(solution);

        if (major.is_finite() || major.is_nan()) && minor.is_finite() {
            Score { major, minor }
        } else {
            Score {
                major: f32::INFINITY,
                minor: f32::INFINITY,
            }
        }
    }

    pub(crate) fn satisfies_constraints_scheduling(scheduling: &Scheduling) -> bool {
        for constraint in &scheduling.input_data.scheduling_constraints {
            let left = constraint.left;
            let right = usize::try_from(constraint.right).unwrap_or(usize::MAX);
            let extra = constraint.extra;

            match constraint.kind {
                crate::ConstraintType::ChoiceIsInSlot => {
                    if scheduling.slot_of(left) != constraint.right {
                        return false;
                    }
                }
                crate::ConstraintType::ChoiceIsNotInSlot => {
                    if scheduling.slot_of(left) == constraint.right {
                        return false;
                    }
                }
                crate::ConstraintType::ChoicesAreInSameSlot => {
                    if scheduling.slot_of(left) != scheduling.slot_of(right) {
                        return false;
                    }
                }
                crate::ConstraintType::ChoicesAreNotInSameSlot => {
                    if scheduling.slot_of(left) != Scheduling::NOT_SCHEDULED
                        && scheduling.slot_of(right) != Scheduling::NOT_SCHEDULED
                        && scheduling.slot_of(left) == scheduling.slot_of(right)
                    {
                        return false;
                    }
                }
                crate::ConstraintType::ChoicesHaveOffset => {
                    let left_slot = scheduling.slot_of(left);
                    let right_slot = scheduling.slot_of(right);
                    if left_slot == Scheduling::NOT_SCHEDULED
                        || right_slot == Scheduling::NOT_SCHEDULED
                    {
                        if left_slot != right_slot {
                            return false;
                        }
                    } else if right_slot - left_slot != extra {
                        return false;
                    }
                }
                crate::ConstraintType::SlotHasLimitedSize => {
                    let slot = i32::try_from(constraint.left).unwrap_or(i32::MAX);
                    let count = i32::try_from(
                        (0..scheduling.input_data.choices.len())
                            .filter(|&choice| scheduling.slot_of(choice) == slot)
                            .count(),
                    )
                    .unwrap_or(i32::MAX);
                    let valid = match extra {
                        1 => count == constraint.right,
                        -1 => count != constraint.right,
                        2 => count > constraint.right,
                        -3 => count < constraint.right,
                        3 => count >= constraint.right,
                        -2 => count <= constraint.right,
                        _ => panic!("Unknown slot size limit operator {extra}."),
                    };
                    if !valid {
                        return false;
                    }
                }
                _ => panic!("Unknown scheduling constraint type {:?}.", constraint.kind),
            }
        }

        true
    }

    pub(crate) fn satisfies_constraints_assignment(
        scheduling: &Scheduling,
        assignment: &Assignment,
    ) -> bool {
        for constraint in &assignment.input_data.assignment_constraints {
            let left = constraint.left;
            let right = usize::try_from(constraint.right).unwrap_or(usize::MAX);

            match constraint.kind {
                crate::ConstraintType::ChoicesHaveSameChoosers => {
                    if scheduling.slot_of(right) != Scheduling::NOT_SCHEDULED
                        && scheduling.slot_of(left) != Scheduling::NOT_SCHEDULED
                        && assignment.choosers_ordered(left) != assignment.choosers_ordered(right)
                    {
                        return false;
                    }
                }
                crate::ConstraintType::ChooserIsInChoice => {
                    if scheduling.slot_of(right) != Scheduling::NOT_SCHEDULED
                        && !assignment.is_in_choice(left, right)
                    {
                        return false;
                    }
                }
                crate::ConstraintType::ChooserIsNotInChoice => {
                    if assignment.is_in_choice(left, right) {
                        return false;
                    }
                }
                crate::ConstraintType::ChoosersHaveSameChoices => {
                    if assignment.choices_ordered(left) != assignment.choices_ordered(right) {
                        return false;
                    }
                }
                _ => panic!("Unknown assignment constraint type {:?}.", constraint.kind),
            }
        }

        true
    }
}

impl Scoring {
    fn evaluate_major(&self, solution: &Solution) -> i32 {
        let assignment = solution.assignment().expect("solution requires assignment");
        let mut max_pref = 0_i32;
        for chooser in 0..self.input_data.choosers.len() {
            for slot in 0..self.input_data.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                max_pref = max_pref.max(self.input_data.choosers[chooser].preferences[choice]);
            }
        }
        max_pref
    }

    fn evaluate_minor(&self, solution: &Solution) -> f32 {
        if !self.is_feasible(solution) {
            return f32::INFINITY;
        }

        let assignment = solution.assignment().expect("solution requires assignment");
        let mut pref_count =
            vec![0_i32; usize::try_from(self.input_data.max_preference + 1).expect("non-negative")];

        for chooser in 0..self.input_data.choosers.len() {
            for slot in 0..self.input_data.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                let pref = usize::try_from(self.input_data.choosers[chooser].preferences[choice])
                    .expect("preference must be non-negative");
                pref_count[pref] += 1;
            }
        }

        let mut sum = 0.0_f32;
        for (pref, &count) in pref_count.iter().enumerate() {
            sum += (count as f32) * (pref as f32).powf(self.options.preference_exponent as f32)
                / self.scaling;
        }
        sum
    }
}
