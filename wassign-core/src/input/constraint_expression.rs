#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum AccessorType {
    NotSet,
    Chooser,
    Choice,
    Slot,
    Size,
    Integer,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum RelationType {
    Eq,
    Neq,
    Gt,
    Lt,
    Geq,
    Leq,
    Contains,
    NotContains,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ConstraintExpressionAccessor {
    pub kind: AccessorType,
    pub sub_type: AccessorType,
    pub name: String,
    pub part: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct ConstraintExpressionRelation {
    pub kind: RelationType,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ConstraintExpression {
    pub left: ConstraintExpressionAccessor,
    pub relation: ConstraintExpressionRelation,
    pub right: ConstraintExpressionAccessor,
}
