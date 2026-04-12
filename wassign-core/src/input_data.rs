use std::collections::BTreeMap;

use crate::{ChoiceData, ChooserData, Constraint, SlotData};

/// Fully parsed and preprocessed input data.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InputData {
    /// Parsed choice definitions in their internal index order.
    pub choices: Vec<ChoiceData>,
    /// Parsed chooser definitions in their internal index order.
    pub choosers: Vec<ChooserData>,
    /// Parsed slot definitions in their internal index order.
    pub slots: Vec<SlotData>,
    /// Scheduling constraints after normalization and preprocessing.
    pub scheduling_constraints: Vec<Constraint>,
    /// Assignment constraints after normalization and preprocessing.
    pub assignment_constraints: Vec<Constraint>,
    /// Groups of choices that must be treated together for dependency handling.
    pub dependent_choice_groups: Vec<Vec<usize>>,
    /// Sorted distinct preference values that occur in the normalized input.
    pub preference_levels: Vec<u32>,
    /// Maximum normalized preference value present in the input.
    pub max_preference: u32,
    /// Lookup table from choice index to relevant scheduling constraints.
    pub choice_constraint_map: BTreeMap<usize, Vec<Constraint>>,
    /// Lookup table from chooser index to relevant assignment constraints.
    pub chooser_constraint_map: BTreeMap<usize, Vec<Constraint>>,
}

impl InputData {
    pub(crate) const GENERATED_PREFIX: &str = "~";
    pub(crate) const GENERATED_SLOT_NAME: &str = "Generated Slot";

    pub(crate) fn preference_after(&self, preference: u32) -> u32 {
        self.preference_levels
            .iter()
            .copied()
            .find(|&level| level > preference)
            .unwrap_or(u32::MAX)
    }

    pub(crate) fn scheduling_constraints_for(&self, choice: usize) -> &[Constraint] {
        self.choice_constraint_map
            .get(&choice)
            .map_or(self.scheduling_constraints.as_slice(), Vec::as_slice)
    }
}
