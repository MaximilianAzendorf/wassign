//! Smoke tests for the large example inputs at the repository root.

mod common;

use std::path::PathBuf;
use std::sync::Arc;

use common::{csa, default_options, parse_data_result, scoring, sd};
use wassign::ShotgunSolverThreaded;

fn root_fixture(name: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join(name)
}

fn smoke_solve_fixture(name: &str) {
    let input = std::fs::read_to_string(root_fixture(name)).expect("fixture should be readable");
    let data = parse_data_result(&input).expect("fixture should parse");
    let mut options = Arc::unwrap_or_clone(default_options());
    options.timeout_seconds = 1;
    let options = Arc::new(options);

    let mut solver = ShotgunSolverThreaded::new(
        data.clone(),
        csa(data.clone(), true),
        sd(&data),
        scoring(data, options.clone()),
        options,
    );
    solver.start().expect("smoke solver should start");
    let _ = solver.wait_for_result().expect("smoke solver should finish");
}

#[test]
fn realistic120_should_parse() {
    let input = std::fs::read_to_string(root_fixture("realistic120.wassign")).expect("fixture should be readable");
    let data = parse_data_result(&input).expect("fixture should parse");

    assert!(data.slot_count() > 1);
    assert!(data.choice_count() >= 10);
    assert!(data.chooser_count() > 10);
}

#[test]
fn realistic300_should_parse() {
    let input = std::fs::read_to_string(root_fixture("realistic300.wassign")).expect("fixture should be readable");
    let data = parse_data_result(&input).expect("fixture should parse");

    assert!(data.slot_count() > 1);
    assert!(data.choice_count() > 10);
    assert!(data.chooser_count() > 10);
}

#[test]
fn realistic120_should_survive_a_short_solver_run() {
    smoke_solve_fixture("realistic120.wassign");
}

#[test]
fn realistic300_should_survive_a_short_solver_run() {
    smoke_solve_fixture("realistic300.wassign");
}
