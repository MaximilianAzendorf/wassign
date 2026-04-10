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
            AccessorType::Slot => {
                ConstraintTarget::Slot(Self::resolve_accessor(data, &expression.right)?)
            }
            AccessorType::Choice => {
                ConstraintTarget::Choice(Self::resolve_accessor(data, &expression.right)?)
            }
            AccessorType::Chooser => {
                ConstraintTarget::Chooser(Self::resolve_accessor(data, &expression.right)?)
            }
            AccessorType::Integer => ConstraintTarget::Limit(
                u32::try_from(Self::resolve_accessor(data, &expression.right)?)
                    .expect("constraint integer must fit in u32"),
            ),
            _ => ConstraintTarget::None,
        };
        result.push(Constraint::new(
            kind,
            Self::resolve_accessor(data, &expression.left)?,
            right,
            extra,
        ));
        Ok(())
    }

    fn resolve_accessor(
        data: &InputData,
        accessor: &ConstraintExpressionAccessor,
    ) -> crate::Result<usize> {
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
                let mut choice = Self::find_name(
                    &accessor.name,
                    &data
                        .choices
                        .iter()
                        .map(|choice| choice.name.clone())
                        .collect::<Vec<_>>(),
                )?;
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
                Ok(choice)
            }
            AccessorType::Integer => accessor
                .name
                .parse::<usize>()
                .map_err(|err| InputError::Message(err.to_string())),
            _ => Err(InputError::Message("Unexpected accessor type.".to_owned())),
        }
    }

    fn find_name(name: &str, values: &[String]) -> crate::Result<usize> {
        let matches = FuzzyMatch::find(name, values);
        match matches.as_slice() {
            [index] => Ok(*index),
            [] => Ok(usize::MAX),
            _ => Err(InputError::Message(format!(
                "The name \"{name}\" is ambiguous."
            ))),
        }
    }
}
