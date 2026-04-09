use crate::SlotData;

use super::input_object::InputObject;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct InputSlotData {
    pub slot: SlotData,
    pub object: InputObject,
}
