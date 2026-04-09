//! Integration tests for the assignment solver.

mod common;

use std::sync::Arc;

use common::*;
use wassign::{AssignmentSolver, Scheduling};

fn assignment_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 0, 100, 50]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [0, 100, 70, 100]);

{extra_constraint}
"#
    )
}

#[test]
fn minimal() {
    let data = parse_data(INPUT_MINIMAL);
    let mut solver = AssignmentSolver::new(data.clone(), csa(data.clone(), true), sd(&data), default_options());

    let scheduling = Arc::new(Scheduling::with_data(data, vec![0]));
    let assignment = solver.solve(scheduling.clone()).expect("expected assignment");

    expect_assignment(&sol(scheduling, assignment), "p,e");
}

#[test]
#[ignore = "Intentionally disabled due to equivalent-solution nondeterminism"]
fn large() {}

#[test]
fn works_without_constraints() {
    let data = parse_data(&assignment_input(""));

    let mut solver = AssignmentSolver::new(data.clone(), csa(data.clone(), false), sd(&data), default_options());
    let scheduling = Arc::new(Scheduling::with_data(data, vec![0, 0, 1, 1]));
    let assignment = solver.solve(scheduling.clone()).expect("expected assignment");

    expect_assignment(&sol(scheduling, assignment), "p1,c1,c3;p2,c1,c3;p3,c2,c4;p4,c2,c4");
}

#[test]
fn choices_have_same_choosers_constraint_works() {
    let data = parse_data(&assignment_input(
        r#"+constraint(choice("c1").choosers == choice("c4").choosers);"#,
    ));

    let mut solver = AssignmentSolver::new(data.clone(), csa(data.clone(), false), sd(&data), default_options());
    let scheduling = Arc::new(Scheduling::with_data(data, vec![0, 0, 1, 1]));
    let assignment = solver.solve(scheduling.clone()).expect("expected assignment");

    expect_assignment(&sol(scheduling, assignment), "p1,c1,c4;p2,c1,c4;p3,c2,c3;p4,c2,c3");
}

#[test]
fn chooser_is_in_choice_constraint_works() {
    let data = parse_data(&assignment_input(
        r#"+constraint(chooser("p1").choices.contains(choice("c4")));"#,
    ));

    let mut solver = AssignmentSolver::new(data.clone(), csa(data.clone(), false), sd(&data), default_options());
    let scheduling = Arc::new(Scheduling::with_data(data, vec![0, 0, 1, 1]));
    let assignment = solver.solve(scheduling.clone()).expect("expected assignment");

    expect_assignment(&sol(scheduling, assignment), "p1,c1,c4;p2,c1,c3;p3,c2,c4;p4,c2,c3");
}

#[test]
fn chooser_is_not_in_choice_constraint_works() {
    let data = parse_data(&assignment_input(
        r#"+constraint(chooser("p1").choices.contains_not(choice("c3")));"#,
    ));

    let mut solver = AssignmentSolver::new(data.clone(), csa(data.clone(), false), sd(&data), default_options());
    let scheduling = Arc::new(Scheduling::with_data(data, vec![0, 0, 1, 1]));
    let assignment = solver.solve(scheduling.clone()).expect("expected assignment");

    expect_assignment(&sol(scheduling, assignment), "p1,c1,c4;p2,c1,c3;p3,c2,c4;p4,c2,c3");
}

#[test]
fn choosers_have_same_choices_constraint_works() {
    let data = parse_data(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 70, 100, 70]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [70, 100, 70, 100]);

+constraint(chooser("p1").choices == chooser("p4").choices);
"#,
    );

    let mut solver = AssignmentSolver::new(data.clone(), csa(data.clone(), false), sd(&data), default_options());
    let scheduling = Arc::new(Scheduling::with_data(data, vec![0, 0, 1, 1]));
    let assignment = solver.solve(scheduling.clone()).expect("expected assignment");

    expect_assignment(&sol(scheduling, assignment), "p1,c1,c3;p2,c2,c4;p3,c2,c4;p4,c1,c3");
}
