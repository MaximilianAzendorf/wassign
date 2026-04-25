#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Score {
    pub(crate) major: f32,
    pub(crate) minor: f32,
}

impl Score {
    pub(crate) fn is_finite(self) -> bool {
        self.minor.is_finite() && (self.major.is_finite() || self.major.is_nan())
    }
}

impl std::fmt::Display for Score {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.major.is_nan() {
            f.write_str(&crate::util::str_float(f64::from(self.minor), 5))
        } else {
            write!(
                f,
                "({}, {})",
                crate::util::str_float(f64::from(self.major), 0),
                crate::util::str_float(f64::from(self.minor), 5)
            )
        }
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
    fn scores_format_for_display() {
        let standard = Score {
            major: 4.0,
            minor: 2.125,
        };
        let greedy = Score {
            major: f32::NAN,
            minor: 2.125,
        };

        assert_eq!(standard.to_string(), "(4, 2.12500)");
        assert_eq!(greedy.to_string(), "2.12500");
    }

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
