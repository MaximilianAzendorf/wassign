#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ProtoChoiceData {
    pub name: String,
    pub min: u32,
    pub max: u32,
    pub parts: u32,
    pub optional: bool,
}
