/// A parsed choice definition.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ChoiceData {
    pub(crate) name: String,
    pub(crate) min: u32,
    pub(crate) max: u32,
    pub(crate) continuation: Option<usize>,
    pub(crate) is_optional: bool,
}

impl ChoiceData {
    /// Returns the choice name.
    #[must_use]
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Returns the minimum number of choosers that must be assigned to this choice.
    #[must_use]
    pub fn min(&self) -> u32 {
        self.min
    }

    /// Returns the maximum number of choosers that may be assigned to this choice.
    #[must_use]
    pub fn max(&self) -> u32 {
        self.max
    }

    /// Returns the continuation choice index for multipart choices, if any.
    #[must_use]
    pub fn continuation(&self) -> Option<usize> {
        self.continuation
    }

    /// Returns whether the choice is optional.
    #[must_use]
    pub fn is_optional(&self) -> bool {
        self.is_optional
    }
}
