use crate::{Constraint, ConstraintExtra, ConstraintTarget, InputData};
use crate::{ConstraintType, InputError, SlotSizeLimitOp};

use super::constraint_expression::{
    AccessorType, ConstraintExpression, ConstraintExpressionAccessor, RelationType,
};
use super::fuzzy_match::FuzzyMatch;

pub struct ConstraintBuilder;

impl ConstraintBuilder {
    pub fn build(
        data: &InputData,
        expression: ConstraintExpression,
    ) -> crate::Result<Vec<Constraint>> {
        let mut expression = expression;
        let mut result = Vec::new();

        // We do to passes so we can adequately handle the mirrored cases
        for pass in 0..2 {
            match (
                expression.left.kind,
                expression.left.sub_type,
                expression.relation.kind,
                expression.right.kind,
                expression.right.sub_type,
            ) {
                (
                    AccessorType::Choice,
                    AccessorType::Slot,
                    RelationType::Eq,
                    AccessorType::Slot,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoiceIsInSlot,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Choice,
                    AccessorType::Slot,
                    RelationType::Neq,
                    AccessorType::Slot,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoiceIsNotInSlot,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Choice,
                    AccessorType::Slot,
                    RelationType::Eq,
                    AccessorType::Choice,
                    AccessorType::Slot,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoicesAreInSameSlot,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Choice,
                    AccessorType::Slot,
                    RelationType::Neq,
                    AccessorType::Choice,
                    AccessorType::Slot,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoicesAreNotInSameSlot,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Slot,
                    AccessorType::Choice,
                    RelationType::Contains,
                    AccessorType::Choice,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::SlotContainsChoice,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Slot,
                    AccessorType::Choice,
                    RelationType::NotContains,
                    AccessorType::Choice,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::SlotNotContainsChoice,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Slot,
                    AccessorType::Choice,
                    RelationType::Eq,
                    AccessorType::Slot,
                    AccessorType::Choice,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::SlotsHaveSameChoices,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Choice,
                    AccessorType::Chooser,
                    RelationType::Eq,
                    AccessorType::Choice,
                    AccessorType::Chooser,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoicesHaveSameChoosers,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Chooser,
                    AccessorType::Choice,
                    RelationType::Contains,
                    AccessorType::Choice,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChooserIsInChoice,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Chooser,
                    AccessorType::Choice,
                    RelationType::NotContains,
                    AccessorType::Choice,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChooserIsNotInChoice,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Chooser,
                    AccessorType::Choice,
                    RelationType::Eq,
                    AccessorType::Chooser,
                    AccessorType::Choice,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoosersHaveSameChoices,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Choice,
                    AccessorType::Chooser,
                    RelationType::Contains,
                    AccessorType::Chooser,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoiceContainsChooser,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Choice,
                    AccessorType::Chooser,
                    RelationType::NotContains,
                    AccessorType::Chooser,
                    AccessorType::NotSet,
                ) => {
                    Self::add_constraint(
                        data,
                        &expression,
                        &mut result,
                        ConstraintType::ChoiceNotContainsChooser,
                        ConstraintExtra::None,
                    )?;
                }
                (
                    AccessorType::Slot,
                    AccessorType::Size,
                    RelationType::Eq
                    | RelationType::Neq
                    | RelationType::Gt
                    | RelationType::Lt
                    | RelationType::Geq
                    | RelationType::Leq,
                    AccessorType::Integer,
                    AccessorType::NotSet,
                ) => Self::add_constraint(
                    data,
                    &expression,
                    &mut result,
                    ConstraintType::SlotHasLimitedSize,
                    ConstraintExtra::SlotSizeLimitOp(match expression.relation.kind {
                        RelationType::Eq => SlotSizeLimitOp::Eq,
                        RelationType::Neq => SlotSizeLimitOp::Neq,
                        RelationType::Gt => SlotSizeLimitOp::Gt,
                        RelationType::Lt => SlotSizeLimitOp::Lt,
                        RelationType::Geq => SlotSizeLimitOp::Geq,
                        RelationType::Leq => SlotSizeLimitOp::Leq,
                        _ => unreachable!(),
                    }),
                )?,
                _ => {
                    if pass == 0 {
                        expression = ConstraintExpression {
                            left: expression.right.clone(),
                            relation: expression.relation,
                            right: expression.left.clone(),
                        };
                        continue;
                    }
                    return Err(InputError::Message("Unsupported constraint.".to_owned()));
                }
            }
            break;
        }

        Ok(result)
    }

    fn add_constraint(
        data: &InputData,
        expression: &ConstraintExpression,
        result: &mut Vec<Constraint>,
        kind: ConstraintType,
        extra: ConstraintExtra,
    ) -> crate::Result<()> {
        let right = match expression.right.kind {
            AccessorType::Slot => match Self::resolve_accessor(data, &expression.right)? {
                Some(slot) => ConstraintTarget::Slot(slot),
                None => return Ok(()),
            },
            AccessorType::Choice => match Self::resolve_accessor(data, &expression.right)? {
                Some(choice) => ConstraintTarget::Choice(choice),
                None => return Ok(()),
            },
            AccessorType::Chooser => match Self::resolve_accessor(data, &expression.right)? {
                Some(chooser) => ConstraintTarget::Chooser(chooser),
                None => return Ok(()),
            },
            AccessorType::Integer => ConstraintTarget::Limit(
                u32::try_from(
                    Self::resolve_accessor(data, &expression.right)?
                        .expect("integer accessors always resolve"),
                )
                .expect("constraint integer must fit in u32"),
            ),
            _ => ConstraintTarget::None,
        };
        let Some(left) = Self::resolve_accessor(data, &expression.left)? else {
            return Ok(());
        };
        result.push(Constraint::new(kind, left, right, extra));
        Ok(())
    }

    fn resolve_accessor(
        data: &InputData,
        accessor: &ConstraintExpressionAccessor,
    ) -> crate::Result<Option<usize>> {
        match accessor.kind {
            AccessorType::Slot => Self::find_name(
                &accessor.name,
                &data
                    .slots
                    .iter()
                    .map(|slot| slot.name.clone())
                    .collect::<Vec<_>>(),
            ),
            AccessorType::Chooser => Self::find_name(
                &accessor.name,
                &data
                    .choosers
                    .iter()
                    .map(|chooser| chooser.name.clone())
                    .collect::<Vec<_>>(),
            ),
            AccessorType::Choice => {
                let Some(mut choice) = Self::find_name(
                    &accessor.name,
                    &data
                        .choices
                        .iter()
                        .map(|choice| choice.name.clone())
                        .collect::<Vec<_>>(),
                )?
                else {
                    return Ok(None);
                };
                let mut part = accessor.part;
                while part > 0 {
                    let Some(next_choice) = data.choices[choice].continuation else {
                        return Err(InputError::Message(format!(
                            "The given choice doesn't have a part {}.",
                            accessor.part
                        )));
                    };
                    choice = next_choice;
                    part -= 1;
                }
                Ok(Some(choice))
            }
            AccessorType::Integer => accessor
                .name
                .parse::<usize>()
                .map(Some)
                .map_err(|err| InputError::Message(err.to_string())),
            _ => Err(InputError::Message("Unexpected accessor type.".to_owned())),
        }
    }

    fn find_name(name: &str, values: &[String]) -> crate::Result<Option<usize>> {
        let matches = FuzzyMatch::find(name, values);
        match matches.as_slice() {
            [index] => Ok(Some(*index)),
            [] => Ok(None),
            _ => Err(InputError::Message(format!(
                "The name \"{name}\" is ambiguous."
            ))),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use crate::{ChoiceData, ChooserData, InputData, SlotData};

    use super::super::constraint_expression::ConstraintExpressionRelation;
    use super::{
        AccessorType, ConstraintBuilder, ConstraintExpression, ConstraintExpressionAccessor,
        RelationType,
    };

    fn accessor(
        kind: AccessorType,
        sub_type: AccessorType,
        name: &str,
    ) -> ConstraintExpressionAccessor {
        ConstraintExpressionAccessor {
            kind,
            sub_type,
            name: name.to_owned(),
            part: 0,
        }
    }

    fn input_data() -> InputData {
        InputData {
            choices: vec![ChoiceData {
                name: "choice".to_owned(),
                min: 1,
                max: 1,
                continuation: None,
                is_optional: false,
            }],
            choosers: vec![ChooserData {
                name: "chooser".to_owned(),
                preferences: vec![0],
            }],
            slots: vec![SlotData {
                name: "slot".to_owned(),
            }],
            scheduling_constraints: Vec::new(),
            assignment_constraints: Vec::new(),
            dependent_choice_groups: Vec::new(),
            preference_levels: Vec::new(),
            max_preference: 0,
            choice_constraint_map: BTreeMap::new(),
            chooser_constraint_map: BTreeMap::new(),
        }
    }

    #[test]
    fn missing_accessor_name_emits_no_constraint() {
        let expression = ConstraintExpression {
            left: accessor(AccessorType::Choice, AccessorType::Slot, "missing"),
            relation: ConstraintExpressionRelation {
                kind: RelationType::Eq,
            },
            right: accessor(AccessorType::Slot, AccessorType::NotSet, "slot"),
        };

        let constraints =
            ConstraintBuilder::build(&input_data(), expression).expect("build should succeed");

        assert!(constraints.is_empty());
    }
}
