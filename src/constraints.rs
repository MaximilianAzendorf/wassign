use crate::Constraint;
use crate::ConstraintType;
use crate::UnionFind;

pub(crate) fn get_dependent_choices(
    constraints: &[Constraint],
    choice_count: usize,
) -> Vec<Vec<usize>> {
    let mut groups = UnionFind::<usize>::new(choice_count);
    for constraint in constraints {
        if constraint.kind == ConstraintType::ChoicesHaveSameChoosers {
            groups.join(
                constraint.left,
                usize::try_from(constraint.right).expect("choice index must be non-negative"),
            );
        }
    }
    groups.groups()
}

pub(crate) fn reduce_and_optimize(
    constraints: &[Constraint],
    choice_count: usize,
) -> (Vec<Constraint>, bool) {
    let mut is_infeasible = false;
    let mut reduced = Vec::new();

    for constraint in constraints {
        let mut new_kind = constraint.kind;
        let mut switch_sides = true;
        let mut add = true;

        match constraint.kind {
            ConstraintType::SlotContainsChoice => new_kind = ConstraintType::ChoiceIsInSlot,
            ConstraintType::SlotNotContainsChoice => new_kind = ConstraintType::ChoiceIsNotInSlot,
            ConstraintType::ChoiceContainsChooser => new_kind = ConstraintType::ChooserIsInChoice,
            ConstraintType::ChoiceNotContainsChooser => {
                new_kind = ConstraintType::ChooserIsNotInChoice;
            }
            ConstraintType::SlotsHaveSameChoices => {
                add = false;
                if i32::try_from(constraint.left).expect("constraint index must fit in i32")
                    != constraint.right
                {
                    is_infeasible = true;
                }
            }
            _ => {
                switch_sides = false;
            }
        }

        if add {
            reduced.push(Constraint::new(
                new_kind,
                if switch_sides {
                    usize::try_from(constraint.right)
                        .expect("constraint index must be non-negative")
                } else {
                    constraint.left
                },
                if switch_sides {
                    i32::try_from(constraint.left).expect("constraint index must fit in i32")
                } else {
                    constraint.right
                },
                constraint.extra,
            ));
        }
    }

    (
        expand_dependent_constraints(&reduced, choice_count),
        is_infeasible,
    )
}

fn expand_dependent_constraints(
    constraints: &[Constraint],
    choice_count: usize,
) -> Vec<Constraint> {
    let mut result = Vec::new();

    let dependent_choices = get_dependent_choices(constraints, choice_count);
    let mandatory_critical_sets = get_mandatory_critical_sets(constraints);

    for group_list in [&dependent_choices, &mandatory_critical_sets] {
        for group in group_list {
            for i in 0..group.len() {
                for j in (i + 1)..group.len() {
                    result.push(Constraint::new(
                        ConstraintType::ChoicesAreNotInSameSlot,
                        group[i],
                        i32::try_from(group[j]).expect("choice index must fit in i32"),
                        0,
                    ));
                }
            }
        }
    }

    for constraint in constraints {
        if !matches!(
            constraint.kind,
            ConstraintType::ChooserIsInChoice | ConstraintType::ChooserIsNotInChoice
        ) {
            continue;
        }

        let mut group = Vec::new();
        for dep_group in &dependent_choices {
            if dep_group.contains(
                &usize::try_from(constraint.right).expect("choice index must be non-negative"),
            ) {
                group.clone_from(dep_group);
                break;
            }
        }

        if group.is_empty() {
            continue;
        }

        for choice in group {
            if i32::try_from(choice).expect("choice index must fit in i32") == constraint.right {
                continue;
            }
            result.push(Constraint::new(
                constraint.kind,
                constraint.left,
                i32::try_from(choice).expect("choice index must fit in i32"),
                0,
            ));
        }
    }

    result.extend(constraints.iter().copied());
    result.sort_by_key(|constraint| {
        (
            constraint.kind as i32,
            constraint.left,
            constraint.right,
            constraint.extra,
        )
    });
    result.dedup();
    result
}

fn get_mandatory_critical_sets(constraints: &[Constraint]) -> Vec<Vec<usize>> {
    let mut groups = std::collections::BTreeMap::<usize, Vec<usize>>::new();
    for constraint in constraints {
        if constraint.kind == ConstraintType::ChooserIsInChoice {
            groups.entry(constraint.left).or_default().push(
                usize::try_from(constraint.right).expect("choice index must be non-negative"),
            );
        }
    }
    groups.into_values().collect()
}
