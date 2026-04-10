use crate::InputData;

/// A scheduling that maps each choice to a slot.
///
/// `data[choice]` stores the slot index as an `i32` because the constraint
/// layer already models slot references with signed integers and `-1` is used
/// as the sentinel for "not scheduled".
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Scheduling {
    pub(crate) input_data: std::sync::Arc<InputData>,
    pub(crate) data: Vec<i32>,
}

impl Scheduling {
    pub(crate) const NOT_SCHEDULED: i32 = -1;

    pub(crate) fn new(input_data: std::sync::Arc<InputData>) -> Self {
        Self {
            input_data,
            data: Vec::new(),
        }
    }

    /// Creates a scheduling from explicit slot assignments.
    ///
    /// # Panics
    ///
    /// Panics if `data.len()` does not match the number of choices in `input_data`.
    #[must_use]
    pub fn with_data(input_data: std::sync::Arc<InputData>, data: Vec<i32>) -> Self {
        assert_eq!(data.len(), input_data.choices.len());
        Self { input_data, data }
    }

    pub(crate) fn is_feasible(&self) -> bool {
        let mut slot_min = vec![0_i32; self.input_data.slots.len()];
        let mut slot_max = vec![0_i32; self.input_data.slots.len()];
        let chooser_count = i32::try_from(self.input_data.choosers.len()).unwrap_or(i32::MAX);

        for (choice_index, &slot) in self.data.iter().enumerate() {
            if slot == Self::NOT_SCHEDULED {
                if !self.input_data.choices[choice_index].is_optional {
                    return false;
                }
                continue;
            }

            let choice = &self.input_data.choices[choice_index];
            let slot = usize::try_from(slot).expect("slot id must be non-negative");
            slot_min[slot] += choice.min;
            slot_max[slot] += choice.max;
        }

        for slot in 0..self.input_data.slots.len() {
            if slot_min[slot] > chooser_count || slot_max[slot] < chooser_count {
                return false;
            }
        }

        crate::Scoring::satisfies_constraints_scheduling(self)
    }

    /// Returns the slot assigned to the given choice.
    ///
    /// Returns `-1` when the choice is not scheduled.
    #[must_use]
    pub fn slot_of(&self, choice: usize) -> i32 {
        self.data[choice]
    }
}
