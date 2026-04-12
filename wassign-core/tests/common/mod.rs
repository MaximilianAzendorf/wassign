#![allow(
    dead_code,
    reason = "shared test helpers are imported selectively by each integration test"
)]

use std::{
    io::Write,
    path::PathBuf,
    process::{Command, ExitStatus, Output, Stdio},
    sync::{Mutex, OnceLock},
    time::{SystemTime, UNIX_EPOCH},
};

use std::collections::BTreeSet;

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

#[derive(Debug)]
pub struct CliRun {
    pub status: ExitStatus,
    pub stdout: String,
    pub stderr: String,
}

impl CliRun {
    pub fn from_output(output: Output) -> Self {
        Self {
            status: output.status,
            stdout: String::from_utf8(output.stdout).expect("stdout should be valid UTF-8"),
            stderr: String::from_utf8(output.stderr).expect("stderr should be valid UTF-8"),
        }
    }

    pub fn assert_success(&self) {
        assert!(
            self.status.success(),
            "expected success.\nstatus: {:?}\nstdout:\n{}\nstderr:\n{}",
            self.status,
            self.stdout,
            self.stderr
        );
    }

    pub fn assert_success_clean(&self) {
        self.assert_success();
        self.assert_no_stderr();
    }

    pub fn assert_failure(&self) {
        assert!(
            !self.status.success(),
            "expected failure.\nstatus: {:?}\nstdout:\n{}\nstderr:\n{}",
            self.status,
            self.stdout,
            self.stderr
        );
    }

    pub fn assert_stdout_exact(&self, expected: &str) {
        assert_eq!(self.stdout, expected, "stderr:\n{}", self.stderr);
    }

    pub fn assert_stderr_exact(&self, expected: &str) {
        assert_eq!(self.stderr, expected, "stdout:\n{}", self.stdout);
    }

    pub fn assert_stdout_contains(&self, needle: &str) {
        assert!(
            self.stdout.contains(needle),
            "expected stdout to contain {:?}.\nstdout:\n{}\nstderr:\n{}",
            needle,
            self.stdout,
            self.stderr
        );
    }

    pub fn assert_stderr_contains(&self, needle: &str) {
        assert!(
            self.stderr.contains(needle),
            "expected stderr to contain {:?}.\nstdout:\n{}\nstderr:\n{}",
            needle,
            self.stdout,
            self.stderr
        );
    }

    pub fn assert_no_stdout(&self) {
        assert!(
            self.stdout.is_empty(),
            "expected no stdout.\nstdout:\n{}\nstderr:\n{}",
            self.stdout,
            self.stderr
        );
    }

    pub fn assert_no_stderr(&self) {
        assert!(
            self.stderr.is_empty(),
            "expected no stderr.\nstdout:\n{}\nstderr:\n{}",
            self.stdout,
            self.stderr
        );
    }

    pub fn parse_stdout_csv(&self) -> ParsedCliCsv {
        ParsedCliCsv::from_stdout(&self.stdout)
    }

    pub fn assert_stdout_differs_from(&self, other: &Self) {
        assert_ne!(
            self.stdout, other.stdout,
            "expected stdout to differ.\nleft stdout:\n{}\nright stdout:\n{}",
            self.stdout, other.stdout
        );
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CsvTable {
    pub header: Vec<String>,
    pub rows: Vec<Vec<String>>,
}

impl CsvTable {
    pub fn row_set(&self) -> BTreeSet<Vec<String>> {
        self.rows.iter().cloned().collect()
    }

    pub fn assert_row_set(&self, expected: &[Vec<&str>]) {
        let expected = expected
            .iter()
            .map(|row| {
                row.iter()
                    .map(|value| (*value).to_owned())
                    .collect::<Vec<_>>()
            })
            .collect::<BTreeSet<_>>();
        assert_eq!(self.row_set(), expected);
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParsedCliCsv {
    pub scheduling: Option<CsvTable>,
    pub assignment: Option<CsvTable>,
}

impl ParsedCliCsv {
    pub fn from_stdout(stdout: &str) -> Self {
        let sections = stdout
            .split("\n\n")
            .filter(|section| !section.trim().is_empty())
            .collect::<Vec<_>>();
        match sections.as_slice() {
            [] => Self {
                scheduling: None,
                assignment: None,
            },
            [assignment] => Self {
                scheduling: None,
                assignment: Some(parse_csv_table(assignment)),
            },
            [scheduling, assignment] => Self {
                scheduling: Some(parse_csv_table(scheduling)),
                assignment: Some(parse_csv_table(assignment)),
            },
            _ => panic!("unexpected stdout CSV sections:\n{stdout}"),
        }
    }
}

fn parse_csv_table(section: &str) -> CsvTable {
    let mut lines = section.lines();
    let header = parse_csv_line(lines.next().expect("CSV section must have a header"));
    let rows = lines.map(parse_csv_line).collect();
    CsvTable { header, rows }
}

fn parse_csv_line(line: &str) -> Vec<String> {
    let mut values = Vec::new();
    let mut current = String::new();
    let mut chars = line.chars().peekable();
    let mut in_quotes = false;

    while let Some(ch) = chars.next() {
        match ch {
            '"' => {
                if in_quotes && chars.peek() == Some(&'"') {
                    current.push('"');
                    chars.next();
                } else {
                    in_quotes = !in_quotes;
                }
            }
            ',' if !in_quotes => {
                values.push(current.trim().to_owned());
                current.clear();
            }
            _ => current.push(ch),
        }
    }
    values.push(current.trim().to_owned());
    values
}

#[derive(Debug)]
pub struct TestDir {
    root: PathBuf,
}

impl TestDir {
    pub fn new(prefix: &str) -> Self {
        let root = temp_file_path(prefix, "");
        std::fs::create_dir(&root).expect("temp test directory should be creatable");
        Self { root }
    }

    pub fn root(&self) -> &std::path::Path {
        &self.root
    }

    pub fn path(&self, relative: &str) -> PathBuf {
        self.root.join(relative)
    }

    pub fn write(&self, relative: &str, contents: &str) -> PathBuf {
        let path = self.path(relative);
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent).expect("parent directories should be creatable");
        }
        std::fs::write(&path, contents).expect("test file should be writable");
        path
    }

    pub fn read(&self, relative: &str) -> String {
        let path = self.path(relative);
        std::fs::read_to_string(&path).unwrap_or_else(|err| {
            panic!("failed to read {}: {err}", path.display());
        })
    }

    pub fn output_prefix(&self, name: &str) -> String {
        self.path(name).to_string_lossy().into_owned()
    }

    pub fn assert_file_exact(&self, relative: &str, expected: &str) {
        assert_eq!(
            self.read(relative),
            expected,
            "file: {}",
            self.path(relative).display()
        );
    }

    pub fn assert_file_contains(&self, relative: &str, needle: &str) {
        let contents = self.read(relative);
        assert!(
            contents.contains(needle),
            "expected {} to contain {:?}.\ncontents:\n{}",
            self.path(relative).display(),
            needle,
            contents
        );
    }

    pub fn assert_file_missing(&self, relative: &str) {
        let path = self.path(relative);
        assert!(
            !path.exists(),
            "expected file to be missing: {}",
            path.display()
        );
    }
}

impl Drop for TestDir {
    fn drop(&mut self) {
        let _ = std::fs::remove_dir_all(&self.root);
    }
}

pub fn run_cli(args: &[&str], stdin: Option<&str>) -> CliRun {
    let _guard = cli_lock()
        .lock()
        .expect("CLI test lock should not be poisoned");
    CliRun::from_output(run_cli_raw(args, stdin))
}

pub fn run_cli_in_dir(dir: &TestDir, args: &[&str], stdin: Option<&str>) -> CliRun {
    let _guard = cli_lock()
        .lock()
        .expect("CLI test lock should not be poisoned");
    CliRun::from_output(run_cli_raw_in_dir(dir.root(), args, stdin))
}

fn cli_lock() -> &'static Mutex<()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(()))
}

fn cli_binary_path() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("../target/debug/wassign")
}

fn run_cli_raw(args: &[&str], stdin: Option<&str>) -> Output {
    let mut command = Command::new(cli_binary_path());
    command
        .env("RUST_LOG", "error")
        .env("RUST_LOG_STYLE", "never")
        .env("WASSIGN_SEED", "1")
        .args(args)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    spawn_and_collect(&mut command, stdin)
}

fn run_cli_raw_in_dir(dir: &std::path::Path, args: &[&str], stdin: Option<&str>) -> Output {
    let mut command = Command::new(cli_binary_path());
    command
        .current_dir(dir)
        .env("RUST_LOG", "error")
        .env("RUST_LOG_STYLE", "never")
        .env("WASSIGN_SEED", "1")
        .args(args)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    spawn_and_collect(&mut command, stdin)
}

fn spawn_and_collect(command: &mut Command, stdin: Option<&str>) -> Output {
    command.stdin(if stdin.is_some() {
        Stdio::piped()
    } else {
        Stdio::null()
    });
    let mut child = command.spawn().expect("cli should spawn");
    if let Some(stdin) = stdin {
        child
            .stdin
            .as_mut()
            .expect("stdin should be piped")
            .write_all(stdin.as_bytes())
            .expect("stdin should be writable");
        let _ = child.stdin.take();
    }

    child.wait_with_output().expect("cli should finish")
}

pub fn expect_assignment(input_data: &InputData, solution: &Solution, expectation: &str) {
    assert_eq!(
        assignment_str(input_data, solution),
        strip_whitespace(expectation)
    );
}

pub fn expect_scheduling(input_data: &InputData, solution: &Solution, expectation: &str) {
    assert_eq!(
        scheduling_str(input_data, solution),
        strip_whitespace(expectation)
    );
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
