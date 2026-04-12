/// A parsed slot definition.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct SlotData {
    pub(crate) name: String,
}

impl SlotData {
    /// Returns the slot name.
    #[must_use]
    pub fn name(&self) -> &str {
        &self.name
    }
}
