use crate::{Constraint, InputData, MipFlow};

pub(crate) type FlowId = u64;

const SLOT_ID_HIGH: i32 = i32::MAX / 2;
const CHOICE_ID_HIGH: i32 = i32::MAX / 2 - 1;

/// Static min-cost-flow data shared across assignment solver invocations.
#[derive(Debug, Clone)]
pub struct MipFlowStaticData {
    pub(crate) base_flow: MipFlow<FlowId, FlowId>,
    pub(crate) blocked_edges: Vec<(usize, usize)>,
    pub(crate) constraints: Vec<Constraint>,
}

impl MipFlowStaticData {
    pub(crate) fn make_long(high: i32, low: i32) -> FlowId {
        (u64::from(high.cast_unsigned()) << 32) | u64::from(low.cast_unsigned())
    }

    pub(crate) fn node_chooser(chooser: usize, slot: usize) -> FlowId {
        Self::make_long(
            i32::try_from(chooser).expect("chooser index must fit in i32"),
            i32::try_from(slot).expect("slot index must fit in i32"),
        )
    }

    pub(crate) fn node_slot(slot: usize) -> FlowId {
        Self::make_long(
            SLOT_ID_HIGH,
            i32::try_from(slot).expect("slot index must fit in i32"),
        )
    }

    pub(crate) fn node_choice(choice: usize) -> FlowId {
        Self::make_long(
            CHOICE_ID_HIGH,
            i32::try_from(choice).expect("choice index must fit in i32"),
        )
    }

    pub(crate) fn edge_id(from: usize, to: usize) -> FlowId {
        Self::make_long(
            i32::try_from(from).expect("node index must fit in i32"),
            i32::try_from(to).expect("node index must fit in i32"),
        )
    }

    /// Builds the reusable flow-graph skeleton for the given input.
    #[must_use]
    pub fn new(input_data: &InputData) -> Self {
        let mut base_flow = MipFlow::default();

        for chooser in 0..input_data.choosers.len() {
            for slot in 0..input_data.slots.len() {
                base_flow.add_keyed_node(Self::node_chooser(chooser, slot));
            }
        }

        for choice in 0..input_data.choices.len() {
            base_flow.add_keyed_node(Self::node_choice(choice));
        }

        for slot in 0..input_data.slots.len() {
            base_flow.add_keyed_node(Self::node_slot(slot));
        }

        Self {
            constraints: input_data.assignment_constraints.clone(),
            base_flow,
            blocked_edges: Vec::new(),
        }
    }
}
