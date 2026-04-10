use crate::{
    ChoiceData, ChooserData, Constraint, ConstraintExtra, ConstraintTarget, ConstraintType,
    InputData, InputError, SlotData,
    UnionFind, constraints,
};

use super::input_reader::InputReader;

#[derive(Debug, Default)]
pub struct InputDataBuilder {
    input_data: Option<InputData>,
}

impl InputDataBuilder {
    pub fn process_input_reader(&mut self, reader: &InputReader) -> crate::Result<()> {
        let mut input_data = InputData {
            choices: Vec::new(),
            choosers: Vec::new(),
            slots: Vec::new(),
            scheduling_constraints: Vec::new(),
            assignment_constraints: Vec::new(),
            dependent_choice_groups: Vec::new(),
            preference_levels: Vec::new(),
            max_preference: 0,
            choice_constraint_map: std::collections::BTreeMap::new(),
            chooser_constraint_map: std::collections::BTreeMap::new(),
        };

        if reader.sets.is_empty() {
            input_data.slots.push(SlotData {
                name: InputData::GENERATED_SLOT_NAME.to_owned(),
            });
        } else {
            input_data
                .slots
                .extend(reader.sets.iter().map(|slot| slot.slot.clone()));
        }

        input_data.choosers.extend(
            reader
                .choosers
                .iter()
                .map(|chooser| chooser.chooser.clone()),
        );

        Self::build_preferences(reader, &mut input_data)?;
        Self::build_preference_levels(reader, &mut input_data);
        Self::compile_choices(reader, &mut input_data);
        Self::build_constraints(reader, &mut input_data)?;
        Self::build_constraint_maps(&mut input_data);

        self.input_data = Some(input_data);
        Ok(())
    }

    pub fn get_input_data(&self) -> InputData {
        self.input_data
            .as_ref()
            .expect("input data builder was not initialized")
            .clone()
    }

    fn build_preferences(reader: &InputReader, input_data: &mut InputData) -> crate::Result<()> {
        input_data.max_preference = 0;
        for chooser in &reader.choosers {
            if chooser.chooser.preferences.len() != reader.choices.len() {
                return Err(InputError::Message(format!(
                    "Wrong number of preferences given for chooser \"{}\".",
                    chooser.chooser.name
                )));
            }
            for &preference in &chooser.chooser.preferences {
                input_data.max_preference = input_data.max_preference.max(preference);
            }
        }

        for chooser in &mut input_data.choosers {
            for preference in &mut chooser.preferences {
                *preference = input_data.max_preference - *preference;
            }
        }

        Ok(())
    }

    fn build_preference_levels(reader: &InputReader, input_data: &mut InputData) {
        for chooser in &reader.choosers {
            input_data
                .preference_levels
                .extend(chooser.chooser.preferences.iter().copied());
        }
        input_data.preference_levels.push(input_data.max_preference);
        input_data.preference_levels.push(0);
        input_data.preference_levels.sort_unstable();
        input_data.preference_levels.dedup();
    }

    fn compile_choices(reader: &InputReader, input_data: &mut InputData) {
        let mut next_index = 0_usize;
        for choice in &reader.choices {
            let proto = &choice.choice;
            input_data.choices.push(ChoiceData {
                name: proto.name.clone(),
                min: proto.min,
                max: proto.max,
                continuation: (proto.parts > 1).then_some(next_index + 1),
                is_optional: proto.optional,
            });

            next_index += 1;
            if proto.parts > 1 {
                for chooser in &mut input_data.choosers {
                    let mut new_prefs = Vec::new();
                    for (index, &pref) in chooser.preferences.clone().iter().enumerate() {
                        let repeat = if index == next_index - 1 {
                            usize::try_from(proto.parts).expect("parts must fit in usize")
                        } else {
                            1
                        };
                        new_prefs.extend(std::iter::repeat_n(pref, repeat));
                    }
                    chooser.preferences = new_prefs;
                }

                for part_index in 1..proto.parts {
                    input_data.choices.push(ChoiceData {
                        name: format!(
                            "{}[{}] {}",
                            InputData::GENERATED_PREFIX,
                            part_index + 1,
                            proto.name
                        ),
                        min: proto.min,
                        max: proto.max,
                        continuation: (part_index != proto.parts - 1).then_some(next_index + 1),
                        is_optional: proto.optional,
                    });
                    next_index += 1;
                }
            }
        }
    }

    fn build_constraints(reader: &InputReader, input_data: &mut InputData) -> crate::Result<()> {
        let mut constraints = Vec::new();
        for expression in &reader.constraint_expressions {
            constraints.extend(super::constraint_builder::ConstraintBuilder::build(
                input_data,
                expression.clone(),
            )?);
        }

        Self::compute_part_constraints(input_data, &mut constraints);
        let (constraints, is_infeasible) =
            constraints::reduce_and_optimize(&constraints, input_data.choices.len());
        if is_infeasible {
            return Err(InputError::Message(
                "The given constraints are not satisfiable.".to_owned(),
            ));
        }

        let new_limits = Self::get_dependent_choice_limits(input_data, &constraints);
        let new_prefs = Self::get_dependent_preferences(input_data, &constraints);

        for (index, choice) in input_data.choices.clone().into_iter().enumerate() {
            input_data.choices[index] = ChoiceData {
                name: choice.name,
                min: new_limits[index].0,
                max: new_limits[index].1,
                continuation: choice.continuation,
                is_optional: choice.is_optional,
            };
        }

        for (index, chooser) in input_data.choosers.clone().into_iter().enumerate() {
            input_data.choosers[index] = ChooserData {
                name: chooser.name,
                preferences: new_prefs[index].clone(),
            };
        }

        for constraint in constraints.iter().copied() {
            if constraint.is_scheduling_constraint() {
                input_data.scheduling_constraints.push(constraint);
            }
            if constraint.is_assignment_constraint() {
                input_data.assignment_constraints.push(constraint);
            }
        }

        input_data.dependent_choice_groups =
            constraints::get_dependent_choices(&constraints, input_data.choices.len());
        Ok(())
    }

    fn compute_part_constraints(input_data: &InputData, constraints: &mut Vec<Constraint>) {
        let mut groups = UnionFind::<usize>::new(input_data.choices.len());
        for (index, choice) in input_data.choices.iter().enumerate() {
            if let Some(next) = choice.continuation {
                groups.join(index, next);
            }
        }

        for mut group in groups.groups() {
            group.sort_unstable();
            for (i, &left) in group.iter().enumerate() {
                for (j, &right) in group.iter().enumerate().skip(i + 1) {
                    constraints.push(Constraint::new(
                        ConstraintType::ChoicesHaveSameChoosers,
                        left,
                        ConstraintTarget::Choice(right),
                        ConstraintExtra::None,
                    ));
                    constraints.push(Constraint::new(
                        ConstraintType::ChoicesHaveOffset,
                        left,
                        ConstraintTarget::Choice(right),
                        ConstraintExtra::Offset(i32::try_from(j - i).expect("offset must fit in i32")),
                    ));
                }
            }
        }
    }

    fn get_dependent_choice_limits(
        input_data: &InputData,
        constraints: &[Constraint],
    ) -> Vec<(u32, u32)> {
        let mut groups = UnionFind::<usize>::new(input_data.choices.len());
        for constraint in constraints {
            if constraint.kind == ConstraintType::ChoicesHaveSameChoosers {
                groups.join(constraint.left, constraint.other_choice());
            }
        }

        let mut limits = vec![(0_u32, u32::MAX); input_data.choices.len()];
        for choice in 0..input_data.choices.len() {
            let group = groups.find(choice);
            limits[group] = (
                limits[group].0.max(input_data.choices[choice].min),
                limits[group].1.min(input_data.choices[choice].max),
            );
        }
        for choice in 0..input_data.choices.len() {
            limits[choice] = limits[groups.find(choice)];
        }
        limits
    }

    fn get_dependent_preferences(
        input_data: &InputData,
        constraints: &[Constraint],
    ) -> Vec<Vec<u32>> {
        let dep_groups = constraints::get_dependent_choices(constraints, input_data.choices.len());
        let mut prefs = input_data
            .choosers
            .iter()
            .map(|chooser| chooser.preferences.clone())
            .collect::<Vec<_>>();

        for prefs_row in prefs.iter_mut().take(input_data.choosers.len()) {
            for group in &dep_groups {
                let min_pref = group
                    .iter()
                    .map(|&choice| prefs_row[choice])
                    .min()
                    .unwrap_or(u32::MAX);
                for &choice in group {
                    prefs_row[choice] = min_pref;
                }
            }
        }

        for constraint in constraints {
            if constraint.kind == ConstraintType::ChooserIsInChoice {
                prefs[constraint.left][constraint.other_choice()] = 0;
            }
        }

        prefs
    }

    fn build_constraint_maps(input_data: &mut InputData) {
        input_data.choice_constraint_map.clear();
        input_data.chooser_constraint_map.clear();
        for choice in 0..input_data.choices.len() {
            input_data.choice_constraint_map.insert(choice, Vec::new());
        }
        for chooser in 0..input_data.choosers.len() {
            input_data
                .chooser_constraint_map
                .insert(chooser, Vec::new());
        }

        for constraint in input_data.scheduling_constraints.iter().copied() {
            match constraint.kind {
                ConstraintType::ChoiceIsInSlot | ConstraintType::ChoiceIsNotInSlot => {
                    input_data
                        .choice_constraint_map
                        .get_mut(&constraint.left)
                        .expect("choice map entry must exist")
                        .push(constraint);
                }
                ConstraintType::ChoicesAreInSameSlot
                | ConstraintType::ChoicesAreNotInSameSlot
                | ConstraintType::ChoicesHaveOffset => {
                    input_data
                        .choice_constraint_map
                        .get_mut(&constraint.left)
                        .expect("choice map entry must exist")
                        .push(constraint);
                    input_data
                        .choice_constraint_map
                        .get_mut(
                            &constraint.other_choice(),
                        )
                        .expect("choice map entry must exist")
                        .push(constraint);
                }
                ConstraintType::ChooserIsInChoice | ConstraintType::ChooserIsNotInChoice => {
                    input_data
                        .chooser_constraint_map
                        .get_mut(&constraint.left)
                        .expect("chooser map entry must exist")
                        .push(constraint);
                }
                ConstraintType::ChoosersHaveSameChoices => {
                    input_data
                        .chooser_constraint_map
                        .get_mut(&constraint.left)
                        .expect("chooser map entry must exist")
                        .push(constraint);
                    input_data
                        .chooser_constraint_map
                        .get_mut(&constraint.other_chooser())
                        .expect("chooser map entry must exist")
                        .push(constraint);
                }
                ConstraintType::ChoicesHaveSameChoosers => {
                    for chooser in 0..input_data.choosers.len() {
                        input_data
                            .chooser_constraint_map
                            .get_mut(&chooser)
                            .expect("chooser map entry must exist")
                            .push(constraint);
                    }
                }
                ConstraintType::SlotHasLimitedSize => {
                    for choice in 0..input_data.choices.len() {
                        input_data
                            .choice_constraint_map
                            .get_mut(&choice)
                            .expect("choice map entry must exist")
                            .push(constraint);
                    }
                }
                _ => panic!("Unknown constraint type {:?}", constraint.kind),
            }
        }
    }
}
