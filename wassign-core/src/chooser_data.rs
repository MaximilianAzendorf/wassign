/// A parsed chooser definition.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ChooserData {
    pub(crate) name: String,
    pub(crate) preferences: Vec<u32>,
}

impl ChooserData {
    /// Returns the chooser name.
    #[must_use]
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Returns the chooser preferences after normalization.
    #[must_use]
    pub fn preferences(&self) -> &[u32] {
        &self.preferences
    }
}
