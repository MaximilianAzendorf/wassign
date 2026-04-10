use crate::InputData;

/// A concrete assignment of choosers to choices for each slot in an input.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Assignment {
    pub(crate) input_data: std::sync::Arc<InputData>,
    pub(crate) data: Vec<Vec<usize>>,
}

impl Assignment {
    /// Creates an assignment for the given input data.
    ///
    /// `data[chooser][slot]` stores the choice assigned to that chooser in that slot.
    ///
    /// # Panics
    ///
    /// Panics if the outer dimension does not match the chooser count or any
    /// inner dimension does not match the slot count.
    #[must_use]
    pub fn new(input_data: std::sync::Arc<InputData>, data: Vec<Vec<usize>>) -> Self {
        assert_eq!(data.len(), input_data.choosers.len());
        for slots in &data {
            assert_eq!(slots.len(), input_data.slots.len());
        }

        Self { input_data, data }
    }

    pub(crate) fn choice_of(&self, chooser: usize, slot: usize) -> usize {
        self.data[chooser][slot]
    }

    pub(crate) fn choosers_ordered(&self, choice: usize) -> Vec<usize> {
        let mut choosers = Vec::new();
        for chooser in 0..self.input_data.choosers.len() {
            for slot in 0..self.input_data.slots.len() {
                if self.choice_of(chooser, slot) == choice {
                    choosers.push(chooser);
                }
            }
        }
        choosers.sort_unstable();
        choosers
    }

    pub(crate) fn choices_ordered(&self, chooser: usize) -> Vec<usize> {
        let mut choices = (0..self.input_data.slots.len())
            .map(|slot| self.choice_of(chooser, slot))
            .collect::<Vec<_>>();
        choices.sort_unstable();
        choices
    }

    pub(crate) fn is_in_choice(&self, chooser: usize, choice: usize) -> bool {
        (0..self.input_data.slots.len()).any(|slot| self.choice_of(chooser, slot) == choice)
    }
}
