use std::collections::BTreeMap;
use std::sync::Arc;

use crate::{CriticalSet, InputData, Status};

/// Critical-set analysis used by the scheduling solver as a heuristic.
#[derive(Debug, Clone)]
pub struct CriticalSetAnalysis {
    pub(crate) sets: Vec<CriticalSet>,
    cached_sets: BTreeMap<i32, Arc<[CriticalSet]>>,
    pub(crate) input_data: Arc<InputData>,
    preference_bound: i32,
    pub(crate) quiet: bool,
}

impl CriticalSetAnalysis {
    /// Builds a new analysis for the given input data.
    ///
    /// When `analyze` is `false`, the instance contains only dummy data and a
    /// conservative preference bound.
    pub fn new(input_data: Arc<InputData>, analyze: bool, simplify: bool) -> Self {
        let mut analysis = Self {
            sets: Vec::new(),
            cached_sets: BTreeMap::new(),
            input_data,
            preference_bound: 0,
            quiet: false,
        };

        if analyze {
            analysis.analyze(simplify);
            analysis.preference_bound = analysis.input_data.max_preference();
            for pref_level in analysis.input_data.preference_levels.iter().copied() {
                let min_size = analysis
                    .sets
                    .iter()
                    .filter(|set| set.preference >= pref_level)
                    .map(CriticalSet::size)
                    .min();
                if min_size.is_some_and(|min_size| min_size >= analysis.input_data.slot_count()) {
                    analysis.preference_bound = analysis.preference_bound.min(pref_level);
                }
            }
            analysis.build_cache();
        }

        analysis
    }

    pub(crate) fn for_preference(&self, preference: i32) -> Arc<[CriticalSet]> {
        self.cached_sets
            .get(&preference)
            .cloned()
            .unwrap_or_else(|| Arc::<[CriticalSet]>::from([]))
    }

    fn build_cache(&mut self) {
        self.cached_sets.clear();
        let mut by_preference = BTreeMap::<i32, Vec<CriticalSet>>::new();
        for set in &self.sets {
            by_preference.entry(set.preference).or_default().push(set.clone());
        }

        let mut minimal_sets = Vec::<CriticalSet>::new();
        for preference in self.input_data.preference_levels.iter().rev().copied() {
            if let Some(sets) = by_preference.get_mut(&preference) {
                sets.sort_by_key(CriticalSet::size);
                for set in sets.iter().cloned() {
                    Self::insert_minimal_set(&mut minimal_sets, set);
                }
            }

            minimal_sets.sort_by_key(CriticalSet::size);
            self.cached_sets.insert(preference, Arc::from(minimal_sets.clone()));
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
    pub fn preference_bound(&self) -> i32 {
        self.preference_bound
    }
}

impl CriticalSetAnalysis {
    fn analyze(&mut self, simplify: bool) {
        Status::debug("Starting critical-set analysis.");
        let analysis_steps = self.input_data.preference_levels.len() * self.input_data.chooser_count();
        let analysis_progress = if self.quiet {
            crate::status::hidden_progress()
        } else {
            crate::status::progress_bar("Critical sets", u64::try_from(analysis_steps).unwrap_or(1))
        };
        let mut pref_index = 0_usize;
        for pref in self.input_data.preference_levels.iter().rev().copied() {
            for chooser in 0..self.input_data.chooser_count() {
                analysis_progress.inc(1);
                analysis_progress.set_message(format!(
                    "p{pref} c{}/{} s{}",
                    chooser + 1,
                    self.input_data.chooser_count(),
                    self.sets.len()
                ));
                let mut set_data = Vec::new();
                let mut min_count = 0_i32;

                for choice in 0..self.input_data.choice_count() {
                    if self.input_data.choosers[chooser].preferences[choice] <= pref {
                        set_data.push(choice);
                        min_count += self.input_data.choices[choice].min;
                    }
                }

                let chooser_slots = self
                    .input_data
                    .chooser_count()
                    .checked_mul(self.input_data.slot_count().saturating_sub(1))
                    .and_then(|value| i32::try_from(value).ok())
                    .expect("chooser-slot product must fit in i32");
                if min_count > chooser_slots {
                    continue;
                }

                let candidate = CriticalSet::new(pref, set_data);
                let is_covered = self.sets.iter().any(|other| candidate.is_covered_by(other));
                if !is_covered {
                    Status::trace(&format!(
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
        Status::info(&format!("Critical-set analysis complete: {} set(s).", self.sets.len()));
        Status::debug(&format!("Critical-set analysis complete: {} set(s).", self.sets.len()));
        Status::debug(&format!("Critical-set analysis produced {} set(s).", self.sets.len()));

        if !simplify {
            return;
        }

        Status::debug("Starting critical-set simplification.");
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
        Status::info(&format!(
            "Critical-set simplification complete: {} set(s).",
            self.sets.len()
        ));
        Status::debug(&format!("Critical-set simplification complete: {} set(s).", self.sets.len()));
        Status::debug(&format!("Simplified critical sets down to {} set(s).", self.sets.len()));
    }
}
