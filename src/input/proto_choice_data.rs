#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ProtoChoiceData {
    pub name: String,
    pub min: i32,
    pub max: i32,
    pub parts: i32,
    pub optional: bool,
}
