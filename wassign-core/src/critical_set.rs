#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct CriticalSet {
    pub(crate) preference: u32,
    pub(crate) data: Vec<usize>,
}

impl CriticalSet {
    pub(crate) fn new(preference: u32, data: Vec<usize>) -> Self {
        Self { preference, data }
    }

    pub(crate) fn is_covered_by(&self, other: &Self) -> bool {
        self.preference <= other.preference && self.is_superset_of(other)
    }

    pub(crate) fn is_superset_of(&self, other: &Self) -> bool {
        if self.data.len() < other.data.len() {
            return false;
        }

        let mut self_index = 0_usize;
        let mut other_index = 0_usize;
        while self_index < self.data.len() && other_index < other.data.len() {
            match self.data[self_index].cmp(&other.data[other_index]) {
                std::cmp::Ordering::Less => self_index += 1,
                std::cmp::Ordering::Equal => {
                    self_index += 1;
                    other_index += 1;
                }
                std::cmp::Ordering::Greater => return false,
            }
        }

        other_index == other.data.len()
    }
    pub(crate) fn size(&self) -> usize {
        self.data.len()
    }
}
