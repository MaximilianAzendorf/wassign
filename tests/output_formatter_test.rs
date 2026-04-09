//! Integration tests for output formatting.

mod common;

use std::sync::Arc;

use common::*;
use wassign::{Assignment, InputError, OutputFormatter, Scheduling};

#[test]
fn scheduling_output_requires_a_scheduling() {
    let data = parse_data(INPUT_MINIMAL);
    let solution = sol_assignment(Arc::new(Assignment::new(data, vec![vec![0]])));

    let error = OutputFormatter::write_scheduling_solution(&solution)
        .expect_err("missing scheduling should be rejected");
    assert!(matches!(error, InputError::IncompleteSolution(_)));
}

#[test]
fn assignment_output_requires_an_assignment() {
    let data = parse_data(INPUT_MINIMAL);
    let solution = sol_scheduling(Arc::new(Scheduling::with_data(data, vec![0])));

    let error = OutputFormatter::write_assignment_solution(&solution)
        .expect_err("missing assignment should be rejected");
    assert!(matches!(error, InputError::IncompleteSolution(_)));
}
