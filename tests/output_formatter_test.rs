//! Integration tests for output formatting.

mod common;

use common::*;
use wassign::{Assignment, InputError, OutputFormatter, Scheduling};

#[test]
fn scheduling_output_requires_a_scheduling() {
    let data = parse_data(INPUT_MINIMAL);
    let solution = sol_assignment(Assignment::new(&data, vec![vec![0]]));

    let error = OutputFormatter::write_scheduling_solution(&data, &solution)
        .expect_err("missing scheduling should be rejected");
    assert!(matches!(error, InputError::IncompleteSolution(_)));
}

#[test]
fn assignment_output_requires_an_assignment() {
    let data = parse_data(INPUT_MINIMAL);
    let solution = sol_scheduling(Scheduling::with_data(&data, vec![Some(0)]));

    let error = OutputFormatter::write_assignment_solution(&data, &solution)
        .expect_err("missing assignment should be rejected");
    assert!(matches!(error, InputError::IncompleteSolution(_)));
}
