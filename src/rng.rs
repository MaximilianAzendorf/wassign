use rand::rngs::StdRng;
use rand::{Rng as _, SeedableRng};
use rand::seq::SliceRandom;
use std::sync::atomic::{AtomicU64, Ordering};

static GLOBAL_SEED: AtomicU64 = AtomicU64::new(0);

/// Pseudo-random number generator used by the solvers.
#[derive(Debug, Clone)]
pub struct Rng {
    engine: StdRng,
}

impl Rng {
    /// Stores the base seed used for new solver-local generators.
    pub fn seed(seed: u64) {
        GLOBAL_SEED.store(seed, Ordering::Relaxed);
    }

    /// Creates a generator from an explicit seed.
    #[must_use]
    pub fn from_seed(seed: u64) -> Self {
        Self {
            engine: StdRng::seed_from_u64(seed),
        }
    }

    /// Creates a generator from the current global seed.
    #[must_use]
    pub fn from_global_seed() -> Self {
        Self::from_seed(Self::global_seed())
    }

    /// Creates a deterministic generator stream derived from the global seed.
    #[must_use]
    pub fn fork(stream: u64) -> Self {
        Self::from_seed(Self::derived_seed(stream))
    }

    /// Derives a deterministic seed stream from the current global seed.
    #[must_use]
    pub fn derived_seed(stream: u64) -> u64 {
        mix_seed(Self::global_seed(), stream)
    }

    /// Returns the current global seed.
    #[must_use]
    pub fn global_seed() -> u64 {
        GLOBAL_SEED.load(Ordering::Relaxed)
    }

    /// Returns the next pseudo-random value.
    #[must_use]
    pub fn sample_i32(&mut self) -> i32 {
        self.engine.random()
    }

    /// Returns a value in `min..max`.
    #[must_use]
    pub fn next_in_range(&mut self, min: i32, max: i32) -> i32 {
        min + (self.sample_i32().rem_euclid(max - min))
    }

    /// Shuffles the provided slice in place.
    pub fn shuffle<T>(&mut self, values: &mut [T]) {
        values.shuffle(&mut self.engine);
    }
}

fn mix_seed(seed: u64, stream: u64) -> u64 {
    let mut value = seed ^ stream.wrapping_add(0x9E37_79B9_7F4A_7C15);
    value = (value ^ (value >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
    value = (value ^ (value >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
    value ^ (value >> 31)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn seeded_generators_are_deterministic() {
        let mut left = Rng::from_seed(123);
        let mut right = Rng::from_seed(123);

        for _ in 0..32 {
            assert_eq!(left.sample_i32(), right.sample_i32());
        }
    }

    #[test]
    fn forked_generators_are_deterministic() {
        Rng::seed(123);
        let mut left = Rng::fork(7);
        let mut right = Rng::fork(7);
        let mut other = Rng::fork(8);

        let mut left_values = Vec::new();
        let mut other_values = Vec::new();
        for _ in 0..16 {
            let left_value = left.sample_i32();
            let right_value = right.sample_i32();
            let other_value = other.sample_i32();
            assert_eq!(left_value, right_value);
            left_values.push(left_value);
            other_values.push(other_value);
        }

        assert_ne!(left_values, other_values);
    }
}
