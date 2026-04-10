use crate::{Constraint, InputData};
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                        0,
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
                    match expression.relation.kind {
                        RelationType::Eq => SlotSizeLimitOp::Eq as i32,
                        RelationType::Neq => SlotSizeLimitOp::Neq as i32,
                        RelationType::Gt => SlotSizeLimitOp::Gt as i32,
                        RelationType::Lt => SlotSizeLimitOp::Lt as i32,
                        RelationType::Geq => SlotSizeLimitOp::Geq as i32,
                        RelationType::Leq => SlotSizeLimitOp::Leq as i32,
                        _ => unreachable!(),
                    },
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
        extra: i32,
    ) -> crate::Result<()> {
        result.push(Constraint::new(
            kind,
            Self::resolve_accessor(data, &expression.left)?,
            i32::try_from(Self::resolve_accessor(data, &expression.right)?)
                .expect("constraint accessor must fit in i32"),
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
