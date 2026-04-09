use crate::ChooserData;

use super::input_object::InputObject;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InputChooserData {
    pub chooser: ChooserData,
    pub object: InputObject,
}
