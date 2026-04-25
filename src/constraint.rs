pub const CONSTRAINT_TYPE_DISCRIMINATION_LIMIT: usize = 1 << 16;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[repr(usize)]
pub enum ConstraintType {
    Invalid = 0,
    ChoiceIsInSlot = 1,
    ChoiceIsNotInSlot = 2,
    ChoicesAreInSameSlot = 3,
    ChoicesAreNotInSameSlot = 4,
    ChoicesHaveOffset = 5,
    SlotHasLimitedSize = 6,
    SlotContainsChoice = 7,
    SlotNotContainsChoice = 8,
    SlotsHaveSameChoices = 9,
    ChoicesHaveSameChoosers = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT,
    ChooserIsInChoice = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT + 1,
    ChooserIsNotInChoice = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT + 2,
    ChoosersHaveSameChoices = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT + 3,
    ChoiceContainsChooser = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT + 4,
    ChoiceNotContainsChooser = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT + 5,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[repr(i32)]
pub enum SlotSizeLimitOp {
    Eq = 1,
    Gt = 2,
    Geq = 3,
    Neq = -1,
    Leq = -2,
    Lt = -3,
}

impl SlotSizeLimitOp {
    #[must_use]
    pub fn negation(self) -> Self {
        match self {
            Self::Eq => Self::Neq,
            Self::Neq => Self::Eq,
            Self::Gt => Self::Leq,
            Self::Geq => Self::Lt,
            Self::Leq => Self::Gt,
            Self::Lt => Self::Geq,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum ConstraintTarget {
    None,
    Slot(usize),
    Choice(usize),
    Chooser(usize),
    Limit(u32),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum ConstraintExtra {
    None,
    Offset(i32),
    SlotSizeLimitOp(SlotSizeLimitOp),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Constraint {
    pub kind: ConstraintType,
    pub left: usize,
    pub right: ConstraintTarget,
    pub extra: ConstraintExtra,
}

impl Constraint {
    pub fn new(
        kind: ConstraintType,
        left: usize,
        right: ConstraintTarget,
        extra: ConstraintExtra,
    ) -> Self {
        Self {
            kind,
            left,
            right,
            extra,
        }
    }

    pub fn negation(self) -> Self {
        let mut neg = self;
        match self.kind {
            ConstraintType::ChoiceIsInSlot => neg.kind = ConstraintType::ChoiceIsNotInSlot,
            ConstraintType::ChoiceIsNotInSlot => neg.kind = ConstraintType::ChoiceIsInSlot,
            ConstraintType::ChoicesAreInSameSlot => {
                neg.kind = ConstraintType::ChoicesAreNotInSameSlot;
            }
            ConstraintType::ChoicesAreNotInSameSlot => {
                neg.kind = ConstraintType::ChoicesAreInSameSlot;
            }
            ConstraintType::SlotHasLimitedSize => {
                neg.extra = ConstraintExtra::SlotSizeLimitOp(self.slot_size_limit_op().negation());
            }
            ConstraintType::SlotContainsChoice => neg.kind = ConstraintType::SlotNotContainsChoice,
            ConstraintType::SlotNotContainsChoice => neg.kind = ConstraintType::SlotContainsChoice,
            ConstraintType::ChooserIsInChoice => neg.kind = ConstraintType::ChooserIsNotInChoice,
            ConstraintType::ChooserIsNotInChoice => neg.kind = ConstraintType::ChooserIsInChoice,
            ConstraintType::ChoiceContainsChooser => {
                neg.kind = ConstraintType::ChoiceNotContainsChooser;
            }
            ConstraintType::ChoiceNotContainsChooser => {
                neg.kind = ConstraintType::ChoiceContainsChooser;
            }
            ConstraintType::Invalid
            | ConstraintType::ChoicesHaveOffset
            | ConstraintType::SlotsHaveSameChoices
            | ConstraintType::ChoicesHaveSameChoosers
            | ConstraintType::ChoosersHaveSameChoices => neg.kind = ConstraintType::Invalid,
        }
        neg
    }

    pub fn is_valid(self) -> bool {
        self.kind != ConstraintType::Invalid
    }

    pub fn is_scheduling_constraint(self) -> bool {
        (self.kind as usize) < CONSTRAINT_TYPE_DISCRIMINATION_LIMIT
            && self.kind != ConstraintType::Invalid
    }

    pub fn is_assignment_constraint(self) -> bool {
        (self.kind as usize) >= CONSTRAINT_TYPE_DISCRIMINATION_LIMIT
            && self.kind != ConstraintType::Invalid
    }

    #[must_use]
    pub fn slot(self) -> usize {
        match self.right {
            ConstraintTarget::Slot(slot) => slot,
            _ => panic!("constraint does not carry a slot target"),
        }
    }

    #[must_use]
    pub fn other_choice(self) -> usize {
        match self.right {
            ConstraintTarget::Choice(choice) => choice,
            _ => panic!("constraint does not carry a choice target"),
        }
    }

    #[must_use]
    pub fn other_chooser(self) -> usize {
        match self.right {
            ConstraintTarget::Chooser(chooser) => chooser,
            _ => panic!("constraint does not carry a chooser target"),
        }
    }

    #[must_use]
    pub fn limit(self) -> u32 {
        match self.right {
            ConstraintTarget::Limit(limit) => limit,
            _ => panic!("constraint does not carry a limit target"),
        }
    }

    #[must_use]
    pub fn offset(self) -> i32 {
        match self.extra {
            ConstraintExtra::Offset(offset) => offset,
            _ => panic!("constraint does not carry an offset"),
        }
    }

    #[must_use]
    pub fn slot_size_limit_op(self) -> SlotSizeLimitOp {
        match self.extra {
            ConstraintExtra::SlotSizeLimitOp(op) => op,
            _ => panic!("constraint does not carry a slot size limit operator"),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{
        CONSTRAINT_TYPE_DISCRIMINATION_LIMIT, Constraint, ConstraintExtra, ConstraintTarget,
        ConstraintType,
    };

    fn constraint(kind: ConstraintType) -> Constraint {
        Constraint::new(kind, 0, ConstraintTarget::None, ConstraintExtra::None)
    }

    #[test]
    fn constraint_type_threshold_classifies_constraints() {
        assert_eq!(CONSTRAINT_TYPE_DISCRIMINATION_LIMIT, 1 << 16);
        assert!(
            (ConstraintType::SlotHasLimitedSize as usize) < CONSTRAINT_TYPE_DISCRIMINATION_LIMIT
        );
        assert!(
            (ConstraintType::ChoicesHaveSameChoosers as usize)
                >= CONSTRAINT_TYPE_DISCRIMINATION_LIMIT
        );

        assert!(constraint(ConstraintType::ChoiceIsInSlot).is_scheduling_constraint());
        assert!(!constraint(ConstraintType::Invalid).is_scheduling_constraint());
        assert!(constraint(ConstraintType::ChooserIsInChoice).is_assignment_constraint());
        assert!(!constraint(ConstraintType::Invalid).is_assignment_constraint());
    }
}
