//! Integration tests for the scheduling solver.

mod common;

use std::sync::Arc;

use common::*;
use wassign::{Assignment, Rng, SchedulingSolver};

fn four_choice_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));

let p = [1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

{extra_constraint}
"#
    )
}

fn five_choice_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 4));
+choice("c2", bounds(1, 4));
+choice("c3", bounds(1, 4));
+choice("c4", bounds(1, 4));
+choice("c5", bounds(1, 4));

let p = [1, 1, 1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);

{extra_constraint}
"#
    )
}

fn common_check(scheduling: &wassign::Scheduling) {
    let mut s1 = 0;
    let mut s2 = 0;
    for w in 0..4 {
        if scheduling.slot_of(w) == 0 {
            s1 += 1;
        } else {
            s2 += 1;
        }
    }

    assert_eq!(s1, 2);
    assert_eq!(s2, 2);
}

#[test]
fn minimal() {
    Rng::seed(12);

    let data = parse_data(INPUT_MINIMAL);
    let mut solver = SchedulingSolver::new(data.clone(), csa(data.clone(), false), default_options());

    assert!(solver.next_scheduling(None));
    let scheduling = solver.scheduling().expect("expected scheduling");
    let assignment = Arc::new(Assignment::new(data, vec![vec![0]]));

    expect_scheduling(&sol(scheduling, assignment), "e,s");
}

#[test]
fn works_without_constraints() {
    Rng::seed(12);
    let data = parse_data(&four_choice_input(""));

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    assert!(solver.next_scheduling(None));
    common_check(&solver.scheduling().expect("expected scheduling"));
}

#[test]
fn choice_is_in_slot_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&four_choice_input(r#"+constraint(choice("c1").slot == slot("s1"));"#));

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        let scheduling = solver.scheduling().expect("expected scheduling");
        assert_eq!(scheduling.slot_of(0), 0);
        common_check(&scheduling);
    }
}

#[test]
fn choice_is_not_in_slot_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&four_choice_input(r#"+constraint(choice("c1").slot != slot("s1"));"#));

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        let scheduling = solver.scheduling().expect("expected scheduling");
        assert_ne!(scheduling.slot_of(0), 0);
        common_check(&scheduling);
    }
}

#[test]
fn choices_are_in_same_slot_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&four_choice_input(r#"+constraint(choice("c1").slot == choice("c3").slot);"#));

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        let scheduling = solver.scheduling().expect("expected scheduling");
        assert_eq!(scheduling.slot_of(0), scheduling.slot_of(2));
        common_check(&scheduling);
    }
}

#[test]
fn choices_are_not_in_same_slot_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&four_choice_input(r#"+constraint(choice("c1").slot != choice("c3").slot);"#));

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        let scheduling = solver.scheduling().expect("expected scheduling");
        assert_ne!(scheduling.slot_of(0), scheduling.slot_of(2));
        common_check(&scheduling);
    }
}

#[test]
fn choices_have_offset_constraint_works() {
    Rng::seed(12);
    let data = parse_data(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2), parts(2));

let p = [1, 1, 1];
+chooser("p1", p);
+chooser("p2", p);
+chooser("p3", p);
+chooser("p4", p);
"#,
    );

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        let scheduling = solver.scheduling().expect("expected scheduling");
        assert_eq!(scheduling.slot_of(2), 0);
        assert_eq!(scheduling.slot_of(3), 1);
        common_check(&scheduling);
    }
}

fn count_slot_zero(scheduling: &wassign::Scheduling, choice_count: usize) -> i32 {
    let mut slot_zero = 0;
    for choice in 0..choice_count {
        if scheduling.slot_of(choice) == 0 {
            slot_zero += 1;
        }
    }
    slot_zero
}

#[test]
fn slot_has_limited_size_eq_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&five_choice_input(r#"+constraint(slot("s1").size == 2);"#));
    let choice_count = data.choice_count();

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        assert_eq!(count_slot_zero(&solver.scheduling().expect("expected scheduling"), choice_count), 2);
    }
}

#[test]
fn slot_has_limited_size_neq_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&five_choice_input(r#"+constraint(slot("s1").size != 2);"#));
    let choice_count = data.choice_count();

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        assert_ne!(count_slot_zero(&solver.scheduling().expect("expected scheduling"), choice_count), 2);
    }
}

#[test]
fn slot_has_limited_size_lt_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&five_choice_input(r#"+constraint(slot("s1").size < 3);"#));
    let choice_count = data.choice_count();

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        assert!(count_slot_zero(&solver.scheduling().expect("expected scheduling"), choice_count) < 3);
    }
}

#[test]
fn slot_has_limited_size_leq_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&five_choice_input(r#"+constraint(slot("s1").size <= 2);"#));
    let choice_count = data.choice_count();

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        assert!(count_slot_zero(&solver.scheduling().expect("expected scheduling"), choice_count) <= 2);
    }
}

#[test]
fn slot_has_limited_size_gt_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&five_choice_input(r#"+constraint(slot("s1").size > 2);"#));
    let choice_count = data.choice_count();

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        assert!(count_slot_zero(&solver.scheduling().expect("expected scheduling"), choice_count) > 2);
    }
}

#[test]
fn slot_has_limited_size_geq_constraint_works() {
    Rng::seed(12);
    let data = parse_data(&five_choice_input(r#"+constraint(slot("s1").size >= 3);"#));
    let choice_count = data.choice_count();

    let mut solver = SchedulingSolver::new(data.clone(), csa(data, false), default_options());
    for _ in 0..16 {
        assert!(solver.next_scheduling(None));
        assert!(count_slot_zero(&solver.scheduling().expect("expected scheduling"), choice_count) >= 3);
    }
}
