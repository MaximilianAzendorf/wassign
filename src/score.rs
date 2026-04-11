#[derive(Debug, Clone, Copy, PartialEq)]
/// Lexicographic objective value of a solution.
pub struct Score {
    pub(crate) major: f32,
    pub(crate) minor: f32,
}

impl Score {
    /// Returns the major score term.
    #[must_use]
    pub fn major(self) -> f32 {
        self.major
    }

    /// Returns the minor score term.
    #[must_use]
    pub fn minor(self) -> f32 {
        self.minor
    }

    /// Formats the score the same way the CLI prints it.
    #[must_use]
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

    /// Returns whether the score is finite and therefore usable.
    #[must_use]
    pub fn is_finite(self) -> bool {
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
        match (self.is_finite(), other.is_finite()) {
            (false, false) => std::cmp::Ordering::Equal,
            (false, true) => std::cmp::Ordering::Greater,
            (true, false) => std::cmp::Ordering::Less,
            (true, true) => {
                let major_cmp = self.major.total_cmp(&other.major);
                if self.major.is_nan()
                    || other.major.is_nan()
                    || major_cmp == std::cmp::Ordering::Equal
                {
                    self.minor.total_cmp(&other.minor)
                } else {
                    major_cmp
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Score;

    #[test]
    fn invalid_scores_are_worse_than_greedy_scores() {
        let invalid = Score {
            major: f32::INFINITY,
            minor: f32::INFINITY,
        };
        let greedy = Score {
            major: f32::NAN,
            minor: 2.0,
        };

        assert!(greedy < invalid);
        assert!(invalid > greedy);
    }

    #[test]
    fn greedy_scores_compare_by_minor_value() {
        let better = Score {
            major: f32::NAN,
            minor: 2.0,
        };
        let worse = Score {
            major: f32::NAN,
            minor: 3.0,
        };

        assert!(better < worse);
    }
}
