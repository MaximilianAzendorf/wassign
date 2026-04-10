//! Integration tests for the assignment solver.

mod common;

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
    let options = default_options();
    let problem = prepared_problem(data.clone(), &options);
    let mut solver = AssignmentSolver::new(&problem, &options);

    let scheduling = Scheduling::with_data(&data, vec![Some(0)]);
    let assignment = solver.solve(&scheduling).expect("expected assignment");

    expect_assignment(&data, &sol(scheduling, assignment), "p,e");
}

#[test]
fn works_without_constraints() {
    let data = parse_data(&assignment_input(""));

    let options = default_options();
    let problem = prepared_problem(data.clone(), &options);
    let mut solver = AssignmentSolver::new(&problem, &options);
    let scheduling = Scheduling::with_data(&data, vec![Some(0), Some(0), Some(1), Some(1)]);
    let assignment = solver.solve(&scheduling).expect("expected assignment");

    expect_assignment(
        &data,
        &sol(scheduling, assignment),
        "p1,c1,c3;p2,c1,c3;p3,c2,c4;p4,c2,c4",
    );
}

#[test]
fn choices_have_same_choosers_constraint_works() {
    let data = parse_data(&assignment_input(
        r#"+constraint(choice("c1").choosers == choice("c4").choosers);"#,
    ));

    let options = default_options();
    let problem = prepared_problem(data.clone(), &options);
    let mut solver = AssignmentSolver::new(&problem, &options);
    let scheduling = Scheduling::with_data(&data, vec![Some(0), Some(0), Some(1), Some(1)]);
    let assignment = solver.solve(&scheduling).expect("expected assignment");

    expect_assignment(
        &data,
        &sol(scheduling, assignment),
        "p1,c1,c4;p2,c1,c4;p3,c2,c3;p4,c2,c3",
    );
}

#[test]
fn chooser_is_in_choice_constraint_works() {
    let data = parse_data(&assignment_input(
        r#"+constraint(chooser("p1").choices.contains(choice("c4")));"#,
    ));

    let options = default_options();
    let problem = prepared_problem(data.clone(), &options);
    let mut solver = AssignmentSolver::new(&problem, &options);
    let scheduling = Scheduling::with_data(&data, vec![Some(0), Some(0), Some(1), Some(1)]);
    let assignment = solver.solve(&scheduling).expect("expected assignment");

    expect_assignment(
        &data,
        &sol(scheduling, assignment),
        "p1,c1,c4;p2,c1,c3;p3,c2,c4;p4,c2,c3",
    );
}

#[test]
fn chooser_is_not_in_choice_constraint_works() {
    let data = parse_data(&assignment_input(
        r#"+constraint(chooser("p1").choices.contains_not(choice("c3")));"#,
    ));

    let options = default_options();
    let problem = prepared_problem(data.clone(), &options);
    let mut solver = AssignmentSolver::new(&problem, &options);
    let scheduling = Scheduling::with_data(&data, vec![Some(0), Some(0), Some(1), Some(1)]);
    let assignment = solver.solve(&scheduling).expect("expected assignment");

    expect_assignment(
        &data,
        &sol(scheduling, assignment),
        "p1,c1,c4;p2,c1,c3;p3,c2,c4;p4,c2,c3",
    );
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

    let options = default_options();
    let problem = prepared_problem(data.clone(), &options);
    let mut solver = AssignmentSolver::new(&problem, &options);
    let scheduling = Scheduling::with_data(&data, vec![Some(0), Some(0), Some(1), Some(1)]);
    let assignment = solver.solve(&scheduling).expect("expected assignment");

    expect_assignment(
        &data,
        &sol(scheduling, assignment),
        "p1,c1,c3;p2,c2,c4;p3,c2,c4;p4,c1,c3",
    );
}
