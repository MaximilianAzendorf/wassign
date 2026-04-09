use std::collections::BTreeMap;

use crate::{ChoiceData, ChooserData, Constraint, SlotData};

/// Fully parsed and preprocessed input data.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InputData {
    pub(crate) choices: Vec<ChoiceData>,
    pub(crate) choosers: Vec<ChooserData>,
    pub(crate) slots: Vec<SlotData>,
    pub(crate) scheduling_constraints: Vec<Constraint>,
    pub(crate) assignment_constraints: Vec<Constraint>,
    pub(crate) dependent_choice_groups: Vec<Vec<usize>>,
    pub(crate) preference_levels: Vec<i32>,
    pub(crate) max_preference: i32,
    pub(crate) choice_constraint_map: BTreeMap<usize, Vec<Constraint>>,
    pub(crate) chooser_constraint_map: BTreeMap<usize, Vec<Constraint>>,
}

impl InputData {
    pub(crate) const GENERATED_PREFIX: &str = "~";
    pub(crate) const GENERATED_SLOT_NAME: &str = "Generated Slot";

    pub(crate) fn preference_after(&self, preference: i32) -> i32 {
        self.preference_levels
            .iter()
            .copied()
            .find(|&level| level > preference)
            .unwrap_or(i32::MAX)
    }

    pub(crate) fn scheduling_constraints_for(&self, choice: usize) -> &[Constraint] {
        self.choice_constraint_map
            .get(&choice)
            .map_or(self.scheduling_constraints.as_slice(), Vec::as_slice)
    }

    /// Returns all scheduling constraints in the input.
    #[must_use]
    pub fn scheduling_constraints(&self) -> &[Constraint] {
        &self.scheduling_constraints
    }

    /// Returns all assignment constraints in the input.
    #[must_use]
    pub fn assignment_constraints(&self) -> &[Constraint] {
        &self.assignment_constraints
    }

    /// Returns the maximum preference value occurring in the input.
    #[must_use]
    pub fn max_preference(&self) -> i32 {
        self.max_preference
    }

    /// Returns the number of choices in the input.
    #[must_use]
    pub fn choice_count(&self) -> usize {
        self.choices.len()
    }

    /// Returns the parsed choices.
    #[must_use]
    pub fn choices(&self) -> &[ChoiceData] {
        &self.choices
    }

    /// Returns the number of choosers in the input.
    #[must_use]
    pub fn chooser_count(&self) -> usize {
        self.choosers.len()
    }

    /// Returns the parsed choosers.
    #[must_use]
    pub fn choosers(&self) -> &[ChooserData] {
        &self.choosers
    }

    /// Returns the number of slots in the input.
    #[must_use]
    pub fn slot_count(&self) -> usize {
        self.slots.len()
    }

    /// Returns the parsed slots.
    #[must_use]
    pub fn slots(&self) -> &[SlotData] {
        &self.slots
    }
}
