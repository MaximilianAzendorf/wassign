#![allow(
    dead_code,
    reason = "shared test helpers are imported selectively by each integration test"
)]

use std::{
    path::PathBuf,
    sync::Arc,
    time::{SystemTime, UNIX_EPOCH},
};

use wassign::{
    Assignment, CriticalSetAnalysis, InputData, InputReader, MipFlowStaticData, Options,
    OutputFormatter, Scheduling, Scoring, ShotgunSolverThreaded, Solution, status,
};

pub const INPUT_MINIMAL: &str = r#"
+slot("s");
+choice("e", bounds(1, 1));
+chooser("p", [1]);
"#;

pub fn parse_data(input: &str) -> Arc<InputData> {
    parse_data_result(input).expect("input should parse")
}

pub fn parse_data_result(input: &str) -> wassign::Result<Arc<InputData>> {
    let options = default_options();
    let mut reader = InputReader::new(&options);
    reader.read_input(input)
}

pub fn default_options() -> Arc<Options> {
    let options = Arc::new(Options {
        thread_count: 13,
        timeout_seconds: 1,
        ..Options::default()
    });
    status::enable_output(&options);
    options
}

pub fn scoring(input_data: Arc<InputData>, options: Arc<Options>) -> Arc<Scoring> {
    Arc::new(Scoring::new(input_data, options))
}

pub fn csa(data: Arc<InputData>, analyze: bool) -> Arc<CriticalSetAnalysis> {
    Arc::new(CriticalSetAnalysis::new(data, analyze, true))
}

pub fn sd(data: &Arc<InputData>) -> Arc<MipFlowStaticData> {
    Arc::new(MipFlowStaticData::new(data.as_ref()))
}

pub fn sol_scheduling(scheduling: Arc<Scheduling>) -> Solution {
    Solution::new(Some(scheduling), None)
}

pub fn sol_assignment(_: Arc<Assignment>) -> Solution {
    Solution::Invalid
}

pub fn sol(scheduling: Arc<Scheduling>, assignment: Arc<Assignment>) -> Solution {
    Solution::new(Some(scheduling), Some(assignment))
}

fn trim_copy(value: &str) -> String {
    value.trim().to_owned()
}

pub fn assignment_str(solution: &Solution) -> String {
    let mut solution_str = OutputFormatter::write_assignment_solution(solution)
        .expect("solution should format")
        .into_string();
    solution_str = solution_str.replace([' ', '"'], "");
    solution_str = solution_str
        .split_once('\n')
        .map_or(solution_str.clone(), |(_, rest)| rest.to_owned());
    trim_copy(&solution_str).replace('\n', ";")
}

pub fn scheduling_str(solution: &Solution) -> String {
    let mut solution_str = OutputFormatter::write_scheduling_solution(solution)
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

pub fn expect_assignment(solution: &Solution, expectation: &str) {
    assert_eq!(assignment_str(solution), strip_whitespace(expectation));
}

pub fn expect_scheduling(solution: &Solution, expectation: &str) {
    assert_eq!(scheduling_str(solution), strip_whitespace(expectation));
}

pub fn solve(data: Arc<InputData>, timeout: i32) -> Solution {
    let mut options = Arc::unwrap_or_clone(default_options());
    options.timeout_seconds = timeout.max(5);
    let options = Arc::new(options);

    let mut solver = ShotgunSolverThreaded::new(
        data.clone(),
        csa(data.clone(), true),
        sd(&data),
        scoring(data, options.clone()),
        options,
    );
    solver.start().expect("solver should start");
    solver.wait_for_result().expect("solver should finish")
}
