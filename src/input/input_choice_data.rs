use super::{input_object::InputObject, proto_choice_data::ProtoChoiceData};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct InputChoiceData {
    pub choice: ProtoChoiceData,
    pub object: InputObject,
}
