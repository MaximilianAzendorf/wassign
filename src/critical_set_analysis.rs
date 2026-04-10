use std::collections::BTreeMap;

use crate::status;
use crate::{CriticalSet, InputData};

/// Critical-set analysis used by the scheduling solver as a heuristic.
#[derive(Debug, Clone)]
pub struct CriticalSetAnalysis {
    pub(crate) sets: Vec<CriticalSet>,
    cached_sets: BTreeMap<u32, Box<[CriticalSet]>>,
    preference_bound: u32,
    pub(crate) quiet: bool,
}

impl CriticalSetAnalysis {
    /// Builds a new analysis for the given input data.
    ///
    /// When `analyze` is `false`, the instance contains only dummy data and a
    /// conservative preference bound.
    ///
    /// # Panics
    ///
    /// Panics if a preference value does not fit in `u32` or `i32` during cache
    /// construction.
    pub fn new(input_data: &InputData, analyze: bool, simplify: bool) -> Self {
        let mut analysis = Self {
            sets: Vec::new(),
            cached_sets: BTreeMap::new(),
            preference_bound: 0,
            quiet: false,
        };

        if analyze {
            analysis.analyze(input_data, simplify);
            analysis.preference_bound = input_data.max_preference;
            for pref_level in input_data.preference_levels.iter().copied() {
                let min_size = analysis
                    .sets
                    .iter()
                    .filter(|set| set.preference >= pref_level)
                    .map(CriticalSet::size)
                    .min();
                if min_size.is_some_and(|min_size| min_size >= input_data.slots.len()) {
                    analysis.preference_bound = analysis.preference_bound.min(pref_level);
                }
            }
            analysis.build_cache(input_data);
        }

        analysis
    }

    pub(crate) fn for_preference(&self, preference: u32) -> &[CriticalSet] {
        self.cached_sets.get(&preference).map_or(&[], Box::as_ref)
    }

    fn build_cache(&mut self, input_data: &InputData) {
        self.cached_sets.clear();
        let mut by_preference = BTreeMap::<u32, Vec<CriticalSet>>::new();
        for set in &self.sets {
            by_preference
                .entry(set.preference)
                .or_default()
                .push(set.clone());
        }

        let mut minimal_sets = Vec::<CriticalSet>::new();
        for preference in input_data.preference_levels.iter().rev().copied() {
            if let Some(sets) = by_preference.get_mut(&preference) {
                sets.sort_by_key(CriticalSet::size);
                for set in sets.iter().cloned() {
                    Self::insert_minimal_set(&mut minimal_sets, set);
                }
            }

            minimal_sets.sort_by_key(CriticalSet::size);
            self.cached_sets
                .insert(preference, minimal_sets.clone().into_boxed_slice());
        }
    }

    fn insert_minimal_set(minimal_sets: &mut Vec<CriticalSet>, candidate: CriticalSet) {
        if minimal_sets.iter().any(|set| candidate.is_superset_of(set)) {
            return;
        }

        minimal_sets.retain(|set| !set.is_superset_of(&candidate));
        minimal_sets.push(candidate);
    }

    /// Returns an upper bound for the lowest preference any full solution can have.
    #[must_use]
    pub fn preference_bound(&self) -> u32 {
        self.preference_bound
    }

    fn analyze(&mut self, input_data: &InputData, simplify: bool) {
        status::debug("Starting critical-set analysis.");
        let analysis_steps = input_data.preference_levels.len() * input_data.choosers.len();
        let analysis_progress = if self.quiet {
            crate::status::hidden_progress()
        } else {
            crate::status::progress_bar("Critical sets", u64::try_from(analysis_steps).unwrap_or(1))
        };
        let mut pref_index = 0_usize;
        for pref in input_data.preference_levels.iter().rev().copied() {
            for chooser in 0..input_data.choosers.len() {
                analysis_progress.inc(1);
                analysis_progress.set_message(format!(
                    "p{pref} c{}/{} s{}",
                    chooser + 1,
                    input_data.choosers.len(),
                    self.sets.len()
                ));
                let mut set_data = Vec::new();
                let mut min_count = 0_u32;

                for choice in 0..input_data.choices.len() {
                    if input_data.choosers[chooser].preferences[choice] <= pref {
                        set_data.push(choice);
                        min_count += input_data.choices[choice].min;
                    }
                }

                let chooser_slots = input_data
                    .choosers
                    .len()
                    .checked_mul(input_data.slots.len().saturating_sub(1))
                    .and_then(|value| u32::try_from(value).ok())
                    .expect("chooser-slot product must fit in u32");
                if min_count > chooser_slots {
                    continue;
                }

                let candidate = CriticalSet::new(pref, set_data);
                let is_covered = self.sets.iter().any(|other| candidate.is_covered_by(other));
                if !is_covered {
                    status::trace(&format!(
                        "Accepted critical set at preference {pref} with {} choice(es).",
                        candidate.size()
                    ));
                    self.sets.push(candidate);
                }
            }
            pref_index += 1;
            let _ = pref_index;
        }
        analysis_progress.finish_and_clear();
        status::info(&format!(
            "Critical-set analysis complete: {} set(s).",
            self.sets.len()
        ));

        if !simplify {
            return;
        }

        status::debug("Starting critical-set simplification.");
        let mut remaining = self.sets.clone();
        let simplification_progress = if self.quiet {
            crate::status::hidden_progress()
        } else {
            crate::status::progress_bar(
                "CS simplify",
                u64::try_from(remaining.len().max(1)).unwrap_or(1),
            )
        };
        let mut index = 0_usize;
        while index < remaining.len() {
            let current = remaining[index].clone();
            remaining.retain(|other| current == *other || !other.is_covered_by(&current));
            index += 1;
            simplification_progress.set_position(u64::try_from(index).unwrap_or(u64::MAX));
            simplification_progress.set_message(format!("left {}", remaining.len()));
        }
        self.sets = remaining;
        simplification_progress.finish_and_clear();
        status::info(&format!(
            "Critical-set simplification complete: {} set(s).",
            self.sets.len()
        ));
    }
}
