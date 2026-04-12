//! Integration tests for the threaded top-level solver.

mod common;

use common::*;

#[test]
fn minimal() {
    let data = parse_data(INPUT_MINIMAL);
    let solution = solve(data.clone(), 1);
    expect_assignment(&data, &solution, "p,e");
    expect_scheduling(&data, &solution, "e,s");
}
