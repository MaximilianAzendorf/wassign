pub struct FuzzyMatch;

impl FuzzyMatch {
    pub fn find(key: &str, values: &[String]) -> Vec<usize> {
        for matches in [
            Self::find_exact(key, values),
            Self::find_by_token(key, values, true),
            Self::find_by_token(key, values, false),
            Self::find_by_substring(key, values),
        ] {
            if !matches.is_empty() {
                return matches;
            }
        }

        Vec::new()
    }

    fn find_exact(key: &str, values: &[String]) -> Vec<usize> {
        values
            .iter()
            .enumerate()
            .filter_map(|(index, value)| (value == key).then_some(index))
            .collect()
    }

    fn find_by_token(key: &str, values: &[String], only_first_token: bool) -> Vec<usize> {
        values
            .iter()
            .enumerate()
            .filter_map(|(index, value)| {
                let mut tokens = value.split(' ').collect::<Vec<_>>();
                if only_first_token {
                    tokens.truncate(1);
                }
                tokens
                    .into_iter()
                    .any(|token| token == key)
                    .then_some(index)
            })
            .collect()
    }

    fn find_by_substring(key: &str, values: &[String]) -> Vec<usize> {
        values
            .iter()
            .enumerate()
            .filter_map(|(index, value)| value.contains(key).then_some(index))
            .collect()
    }
}
