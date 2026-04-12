use crate::{Assignment, InputData, Options, Scheduling, Score, Solution};

/// Evaluates complete solutions according to feasibility and preference quality.
#[derive(Debug, Clone)]
pub struct Scoring {
    greedy: bool,
    minor_preference_scores: Vec<f32>,
    assignment_preference_costs: Vec<i64>,
}

impl Scoring {
    /// Creates a new scorer for the given input and options.
    #[must_use]
    pub fn new(input_data: &InputData, options: &Options) -> Self {
        let scaling = if input_data.max_preference == 0 {
            1.0
        } else {
            (input_data.max_preference as f32).powf(options.preference_exponent as f32)
        };
        let preference_count = usize::try_from(input_data.max_preference + 1)
            .expect("preference range must fit in usize");
        let minor_preference_scores = (0..preference_count)
            .map(|pref| (pref as f32).powf(options.preference_exponent as f32) / scaling)
            .collect();
        let assignment_preference_costs = (0..preference_count)
            .map(|pref| ((pref as f64 + 1.0).powf(options.preference_exponent)) as i64)
            .collect();
        Self {
            greedy: options.greedy,
            minor_preference_scores,
            assignment_preference_costs,
        }
    }

    pub(crate) fn assignment_preference_cost(&self, preference: u32) -> i64 {
        self.assignment_preference_costs
            [usize::try_from(preference).expect("preference must fit in usize")]
    }

    pub(crate) fn is_feasible(input_data: &InputData, solution: &Solution) -> bool {
        let scheduling = solution
            .scheduling()
            .expect("feasibility requires a scheduling");
        let assignment = solution
            .assignment()
            .expect("feasibility requires an assignment");

        let mut part_counts = vec![0_u32; input_data.choices.len()];
        let mut is_in_slot = vec![vec![false; input_data.slots.len()]; input_data.choosers.len()];
        let slots = (0..input_data.choices.len())
            .map(|choice| scheduling.slot_of(choice))
            .collect::<Vec<_>>();

        if !Self::satisfies_constraints_scheduling(input_data, scheduling)
            || !Self::satisfies_constraints_assignment(input_data, scheduling, assignment)
        {
            return false;
        }

        for (chooser, slots_in) in is_in_slot
            .iter_mut()
            .enumerate()
            .take(input_data.choosers.len())
        {
            for slot in 0..input_data.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                let scheduled_slot = slots[choice].expect("choice must be scheduled");
                if slots_in[scheduled_slot] {
                    return false;
                }
                slots_in[scheduled_slot] = true;
                part_counts[choice] += 1;
            }
        }

        for (choice_index, &count) in part_counts.iter().enumerate() {
            if scheduling.slot_of(choice_index).is_none() {
                continue;
            }
            let choice = &input_data.choices[choice_index];
            if count < choice.min || count > choice.max {
                return false;
            }
        }

        true
    }

    /// Evaluates a solution and returns its score.
    #[must_use]
    pub fn evaluate(&self, input_data: &InputData, solution: &Solution) -> Score {
        if solution.is_invalid() {
            return Score {
                major: f32::INFINITY,
                minor: f32::INFINITY,
            };
        }

        let major = if self.greedy {
            f32::NAN
        } else {
            Self::evaluate_major(input_data, solution) as f32
        };
        let minor = self.evaluate_minor(input_data, solution);

        if (major.is_finite() || major.is_nan()) && minor.is_finite() {
            Score { major, minor }
        } else {
            Score {
                major: f32::INFINITY,
                minor: f32::INFINITY,
            }
        }
    }

    pub(crate) fn satisfies_constraints_scheduling(
        input_data: &InputData,
        scheduling: &Scheduling,
    ) -> bool {
        for constraint in &input_data.scheduling_constraints {
            let left = constraint.left;

            match constraint.kind {
                crate::ConstraintType::ChoiceIsInSlot => {
                    if scheduling.slot_of(left) != Some(constraint.slot()) {
                        return false;
                    }
                }
                crate::ConstraintType::ChoiceIsNotInSlot => {
                    if scheduling.slot_of(left) == Some(constraint.slot()) {
                        return false;
                    }
                }
                crate::ConstraintType::ChoicesAreInSameSlot => {
                    if scheduling.slot_of(left) != scheduling.slot_of(constraint.other_choice()) {
                        return false;
                    }
                }
                crate::ConstraintType::ChoicesAreNotInSameSlot => {
                    if scheduling.slot_of(left).is_some()
                        && scheduling.slot_of(constraint.other_choice()).is_some()
                        && scheduling.slot_of(left) == scheduling.slot_of(constraint.other_choice())
                    {
                        return false;
                    }
                }
                crate::ConstraintType::ChoicesHaveOffset => {
                    let left_slot = scheduling.slot_of(left);
                    let right_slot = scheduling.slot_of(constraint.other_choice());
                    match (left_slot, right_slot) {
                        (Some(left_slot), Some(right_slot)) => {
                            if i32::try_from(right_slot).expect("slot fits in i32")
                                - i32::try_from(left_slot).expect("slot fits in i32")
                                != constraint.offset()
                            {
                                return false;
                            }
                        }
                        (None, None) => {}
                        _ => return false,
                    }
                }
                crate::ConstraintType::SlotHasLimitedSize => {
                    let count = u32::try_from(
                        (0..input_data.choices.len())
                            .filter(|&choice| scheduling.slot_of(choice) == Some(constraint.left))
                            .count(),
                    )
                    .unwrap_or(u32::MAX);
                    let valid = match constraint.slot_size_limit_op() {
                        crate::SlotSizeLimitOp::Eq => count == constraint.limit(),
                        crate::SlotSizeLimitOp::Neq => count != constraint.limit(),
                        crate::SlotSizeLimitOp::Gt => count > constraint.limit(),
                        crate::SlotSizeLimitOp::Lt => count < constraint.limit(),
                        crate::SlotSizeLimitOp::Geq => count >= constraint.limit(),
                        crate::SlotSizeLimitOp::Leq => count <= constraint.limit(),
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
        input_data: &InputData,
        scheduling: &Scheduling,
        assignment: &Assignment,
    ) -> bool {
        for constraint in &input_data.assignment_constraints {
            let left = constraint.left;

            match constraint.kind {
                crate::ConstraintType::ChoicesHaveSameChoosers => {
                    if scheduling.slot_of(constraint.other_choice()).is_some()
                        && scheduling.slot_of(left).is_some()
                        && assignment.choosers_ordered(input_data, left)
                            != assignment.choosers_ordered(input_data, constraint.other_choice())
                    {
                        return false;
                    }
                }
                crate::ConstraintType::ChooserIsInChoice => {
                    if scheduling.slot_of(constraint.other_choice()).is_some()
                        && !assignment.is_in_choice(input_data, left, constraint.other_choice())
                    {
                        return false;
                    }
                }
                crate::ConstraintType::ChooserIsNotInChoice => {
                    if assignment.is_in_choice(input_data, left, constraint.other_choice()) {
                        return false;
                    }
                }
                crate::ConstraintType::ChoosersHaveSameChoices => {
                    if assignment.choices_ordered(input_data, left)
                        != assignment.choices_ordered(input_data, constraint.other_chooser())
                    {
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
    fn evaluate_major(input_data: &InputData, solution: &Solution) -> u32 {
        let assignment = solution.assignment().expect("solution requires assignment");
        let mut max_pref = 0_u32;
        for chooser in 0..input_data.choosers.len() {
            for slot in 0..input_data.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                max_pref = max_pref.max(input_data.choosers[chooser].preferences[choice]);
            }
        }
        max_pref
    }

    fn evaluate_minor(&self, input_data: &InputData, solution: &Solution) -> f32 {
        if !Self::is_feasible(input_data, solution) {
            return f32::INFINITY;
        }

        let assignment = solution.assignment().expect("solution requires assignment");
        let mut pref_count = vec![
            0_u32;
            usize::try_from(input_data.max_preference + 1)
                .expect("preference range must fit in usize")
        ];

        for chooser in 0..input_data.choosers.len() {
            for slot in 0..input_data.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                let pref = usize::try_from(input_data.choosers[chooser].preferences[choice])
                    .expect("preference must fit in usize");
                pref_count[pref] += 1;
            }
        }

        let mut sum = 0.0_f32;
        for (pref, &count) in pref_count.iter().enumerate() {
            sum += (count as f32) * self.minor_preference_scores[pref];
        }
        sum
    }
}
