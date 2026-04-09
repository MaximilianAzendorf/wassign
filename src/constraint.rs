pub const CONSTRAINT_TYPE_DISCRIMINATION_LIMIT: i32 = 1 << 16;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[repr(i32)]
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

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Constraint {
    pub kind: ConstraintType,
    pub left: usize,
    pub right: i32,
    pub extra: i32,
}

impl Constraint {
    pub fn new(kind: ConstraintType, left: usize, right: i32, extra: i32) -> Self {
        Self { kind, left, right, extra }
    }

    pub fn negation(self) -> Self {
        let mut neg = self;
        match self.kind {
            ConstraintType::ChoiceIsInSlot => neg.kind = ConstraintType::ChoiceIsNotInSlot,
            ConstraintType::ChoiceIsNotInSlot => neg.kind = ConstraintType::ChoiceIsInSlot,
            ConstraintType::ChoicesAreInSameSlot => neg.kind = ConstraintType::ChoicesAreNotInSameSlot,
            ConstraintType::ChoicesAreNotInSameSlot => neg.kind = ConstraintType::ChoicesAreInSameSlot,
            ConstraintType::SlotHasLimitedSize => neg.extra = -neg.extra,
            ConstraintType::SlotContainsChoice => neg.kind = ConstraintType::SlotNotContainsChoice,
            ConstraintType::SlotNotContainsChoice => neg.kind = ConstraintType::SlotContainsChoice,
            ConstraintType::ChooserIsInChoice => neg.kind = ConstraintType::ChooserIsNotInChoice,
            ConstraintType::ChooserIsNotInChoice => neg.kind = ConstraintType::ChooserIsInChoice,
            ConstraintType::ChoiceContainsChooser => neg.kind = ConstraintType::ChoiceNotContainsChooser,
            ConstraintType::ChoiceNotContainsChooser => neg.kind = ConstraintType::ChoiceContainsChooser,
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
        (self.kind as i32) < CONSTRAINT_TYPE_DISCRIMINATION_LIMIT && self.kind != ConstraintType::Invalid
    }

    pub fn is_assignment_constraint(self) -> bool {
        (self.kind as i32) >= CONSTRAINT_TYPE_DISCRIMINATION_LIMIT && self.kind != ConstraintType::Invalid
    }
}
