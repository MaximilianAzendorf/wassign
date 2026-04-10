use crate::InputData;

/// A scheduling that maps each choice to a slot.
///
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Scheduling(Vec<Option<usize>>);

impl Scheduling {
    pub(crate) fn new() -> Self {
        Self(Vec::new())
    }

    /// Creates a scheduling from explicit slot assignments.
    ///
    /// # Panics
    ///
    /// Panics if `data.len()` does not match the number of choices in `input_data`.
    #[must_use]
    pub fn with_data(input_data: &InputData, data: Vec<Option<usize>>) -> Self {
        assert_eq!(data.len(), input_data.choices.len());
        Self(data)
    }

    pub(crate) fn is_feasible(&self, input_data: &InputData) -> bool {
        let mut slot_min = vec![0_u32; input_data.slots.len()];
        let mut slot_max = vec![0_u32; input_data.slots.len()];
        let chooser_count = u32::try_from(input_data.choosers.len()).unwrap_or(u32::MAX);

        for (choice_index, slot) in self.0.iter().copied().enumerate() {
            let Some(slot) = slot else {
                if !input_data.choices[choice_index].is_optional {
                    return false;
                }
                continue;
            };

            let choice = &input_data.choices[choice_index];
            slot_min[slot] += choice.min;
            slot_max[slot] += choice.max;
        }

        for slot in 0..input_data.slots.len() {
            if slot_min[slot] > chooser_count || slot_max[slot] < chooser_count {
                return false;
            }
        }

        crate::Scoring::satisfies_constraints_scheduling(input_data, self)
    }

    /// Returns the slot assigned to the given choice.
    ///
    /// Returns `None` when the choice is not scheduled.
    #[must_use]
    pub fn slot_of(&self, choice: usize) -> Option<usize> {
        self.0[choice]
    }

    pub(crate) fn to_data(&self) -> Vec<Option<usize>> {
        self.0.clone()
    }
}
