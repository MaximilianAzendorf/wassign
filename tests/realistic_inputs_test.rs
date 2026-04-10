//! Smoke tests for the large example inputs in `tests/inputs`.

mod common;

use std::path::PathBuf;

use common::{default_options, parse_data_result, prepared_problem};
use wassign::ThreadedSolver;

fn root_fixture(name: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests/inputs")
        .join(name)
}

fn smoke_solve_fixture(name: &str) {
    let input = std::fs::read_to_string(root_fixture(name)).expect("fixture should be readable");
    let data = parse_data_result(&input).expect("fixture should parse");
    let mut options = default_options();
    options.timeout_seconds = 1;

    let solver = ThreadedSolver::new(prepared_problem(data, &options), options);
    let _ = solver
        .start()
        .expect("smoke solver should start")
        .wait()
        .expect("smoke solver should finish");
}

#[test]
fn realistic120_should_parse() {
    let input = std::fs::read_to_string(root_fixture("realistic120.wassign"))
        .expect("fixture should be readable");
    let data = parse_data_result(&input).expect("fixture should parse");

    assert!(data.slots.len() > 1);
    assert!(data.choices.len() >= 10);
    assert!(data.choosers.len() > 10);
}

#[test]
fn realistic300_should_parse() {
    let input = std::fs::read_to_string(root_fixture("realistic300.wassign"))
        .expect("fixture should be readable");
    let data = parse_data_result(&input).expect("fixture should parse");

    assert!(data.slots.len() > 1);
    assert!(data.choices.len() > 10);
    assert!(data.choosers.len() > 10);
}

#[test]
fn realistic120_should_survive_a_short_solver_run() {
    smoke_solve_fixture("realistic120.wassign");
}

#[test]
fn realistic300_should_survive_a_short_solver_run() {
    smoke_solve_fixture("realistic300.wassign");
}
