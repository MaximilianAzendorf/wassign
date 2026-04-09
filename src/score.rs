#![expect(clippy::float_cmp, reason = "score ordering intentionally treats equal and paired-NaN major components specially")]

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Score {
    pub(crate) major: f32,
    pub(crate) minor: f32,
}

impl Score {
    pub fn to_str(self) -> String {
        if self.major.is_nan() {
            crate::util::str_float(f64::from(self.minor), 5)
        } else {
            format!(
                "({}, {})",
                crate::util::str_float(f64::from(self.major), 0),
                crate::util::str_float(f64::from(self.minor), 5)
            )
        }
    }

    pub(crate) fn is_finite(self) -> bool {
        self.minor.is_finite() && (self.major.is_finite() || self.major.is_nan())
    }
}

impl Eq for Score {}

impl PartialOrd for Score {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Score {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        if other.major.is_infinite() && other.minor.is_infinite() {
            return std::cmp::Ordering::Less;
        }

        if self.major == other.major || (self.major.is_nan() && other.major.is_nan()) {
            self.minor.total_cmp(&other.minor)
        } else {
            self.major.total_cmp(&other.major)
        }
    }
}
