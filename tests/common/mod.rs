#![allow(
    dead_code,
    reason = "shared test helpers are imported selectively by each integration test"
)]

use std::{
    path::PathBuf,
    time::{SystemTime, UNIX_EPOCH},
};

use wassign::{
    Assignment, InputData, InputReader, Options, OutputFormatter, PreparedProblem, Scheduling,
    Solution, ThreadedSolver, status,
};

pub const INPUT_MINIMAL: &str = r#"
+slot("s");
+choice("e", bounds(1, 1));
+chooser("p", [1]);
"#;

pub fn parse_data(input: &str) -> InputData {
    parse_data_result(input).expect("input should parse")
}

pub fn parse_data_result(input: &str) -> wassign::Result<InputData> {
    let options = default_options();
    let mut reader = InputReader::new(&options);
    reader.read_input(input)
}

pub fn default_options() -> Options {
    let options = Options {
        thread_count: 13,
        timeout_seconds: 1,
        ..Options::default()
    };
    status::enable_output(&options);
    options
}

pub fn prepared_problem(input_data: InputData, options: &Options) -> PreparedProblem {
    PreparedProblem::new(input_data, options)
}

pub fn sol_scheduling(scheduling: Scheduling) -> Solution {
    Solution::new(Some(scheduling), None)
}

pub fn sol_assignment(_: Assignment) -> Solution {
    Solution::Invalid
}

pub fn sol(scheduling: Scheduling, assignment: Assignment) -> Solution {
    Solution::new(Some(scheduling), Some(assignment))
}

fn trim_copy(value: &str) -> String {
    value.trim().to_owned()
}

pub fn assignment_str(input_data: &InputData, solution: &Solution) -> String {
    let mut solution_str = OutputFormatter::write_assignment_solution(input_data, solution)
        .expect("solution should format")
        .into_string();
    solution_str = solution_str.replace([' ', '"'], "");
    solution_str = solution_str
        .split_once('\n')
        .map_or(solution_str.clone(), |(_, rest)| rest.to_owned());
    trim_copy(&solution_str).replace('\n', ";")
}

pub fn scheduling_str(input_data: &InputData, solution: &Solution) -> String {
    let mut solution_str = OutputFormatter::write_scheduling_solution(input_data, solution)
        .expect("solution should format")
        .into_string();
    solution_str = solution_str.replace([' ', '"'], "");
    solution_str = solution_str
        .split_once('\n')
        .map_or(solution_str.clone(), |(_, rest)| rest.to_owned());
    trim_copy(&solution_str).replace('\n', ";")
}

pub fn strip_whitespace(text: &str) -> String {
    text.replace([' ', '\t', '\n'], "")
}

pub fn temp_file_path(prefix: &str, suffix: &str) -> PathBuf {
    let stamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after UNIX_EPOCH")
        .as_nanos();
    std::env::temp_dir().join(format!("wassign-{prefix}-{stamp}{suffix}"))
}

pub fn write_temp_file(prefix: &str, suffix: &str, contents: &str) -> PathBuf {
    let path = temp_file_path(prefix, suffix);
    std::fs::write(&path, contents).expect("temp file should be writable");
    path
}

pub fn expect_assignment(input_data: &InputData, solution: &Solution, expectation: &str) {
    assert_eq!(assignment_str(input_data, solution), strip_whitespace(expectation));
}

pub fn expect_scheduling(input_data: &InputData, solution: &Solution, expectation: &str) {
    assert_eq!(scheduling_str(input_data, solution), strip_whitespace(expectation));
}

pub fn solve(data: InputData, timeout: i32) -> Solution {
    let mut options = default_options();
    options.timeout_seconds = timeout.max(5);

    let solver = ThreadedSolver::new(prepared_problem(data, &options), options);
    solver
        .start()
        .expect("solver should start")
        .wait()
        .expect("solver should finish")
        .into_solution()
}
