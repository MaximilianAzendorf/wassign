//! Integration tests for the Rust CLI surface.

mod common;

use std::io::Write;
use std::process::{Command, Output, Stdio};

use common::{temp_file_path, write_temp_file};
use wassign::WASSIGN_VERSION;

fn run_cli(args: &[&str], stdin: Option<&str>) -> Output {
    let mut command = Command::new(env!("CARGO_BIN_EXE_wassign"));
    command.args(args).stdout(Stdio::piped()).stderr(Stdio::piped());
    if stdin.is_some() {
        command.stdin(Stdio::piped());
    }
    let mut child = command.spawn().expect("cli should spawn");
    if let Some(stdin) = stdin {
        child
            .stdin
            .as_mut()
            .expect("stdin should be piped")
            .write_all(stdin.as_bytes())
            .expect("stdin should be writable");
    }
    child.wait_with_output().expect("cli should finish")
}

#[test]
fn help_lists_supported_flags_without_verbosity() {
    let output = run_cli(&["--help"], None);

    assert!(output.status.success());
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(stdout.contains(&format!("wassign [Version {WASSIGN_VERSION}]")));
    assert!(stdout.contains("--max-neighbors"));
    assert!(stdout.contains("--no-cs-simp"));
    assert!(!stdout.contains("--verbosity"));
    assert!(!stdout.contains(" -v"));
}

#[test]
fn version_reports_the_binary_version() {
    let output = run_cli(&["--version"], None);

    assert!(output.status.success());
    assert_eq!(String::from_utf8_lossy(&output.stdout).trim(), WASSIGN_VERSION);
}

#[test]
fn legacy_verbosity_flag_is_rejected() {
    let output = run_cli(&["-v"], None);

    assert!(!output.status.success());
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(stderr.contains("-v"));
    assert!(stderr.contains("Invalid arguments."));
}

#[test]
fn output_prefix_writes_expected_csv_files() {
    let input_path = write_temp_file(
        "cli-input",
        ".wassign",
        r#"
+slot("s1");
+slot("s2");
let c1 = +choice("c1", bounds(2, 2));
let c2 = +choice("c2", bounds(2, 2));
+constraint(c1.slot == slot("s1"));
+constraint(c2.slot == slot("s2"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
    );
    let output_prefix = temp_file_path("cli-output", "");
    let output_prefix = output_prefix.to_string_lossy().into_owned();

    let output = run_cli(
        &["--input", &input_path.to_string_lossy(), "--output", &output_prefix, "--timeout", "1s"],
        None,
    );

    assert!(output.status.success(), "stderr: {}", String::from_utf8_lossy(&output.stderr));

    let scheduling_path = format!("{output_prefix}.scheduling.csv");
    let assignment_path = format!("{output_prefix}.assignment.csv");
    let scheduling = std::fs::read_to_string(&scheduling_path).expect("scheduling file should exist");
    let assignment = std::fs::read_to_string(&assignment_path).expect("assignment file should exist");

    assert_eq!(
        scheduling,
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\""
    );
    assert_eq!(
        assignment,
        "\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\""
    );

    let _ = std::fs::remove_file(input_path);
    let _ = std::fs::remove_file(scheduling_path);
    let _ = std::fs::remove_file(assignment_path);
}

#[test]
fn stdout_contains_both_csv_documents_without_an_output_prefix() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
let c1 = +choice("c1", bounds(2, 2));
let c2 = +choice("c2", bounds(2, 2));
+constraint(c1.slot == slot("s1"));
+constraint(c2.slot == slot("s2"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
        ),
    );

    assert!(output.status.success(), "stderr: {}", String::from_utf8_lossy(&output.stderr));
    assert_eq!(
        String::from_utf8_lossy(&output.stdout),
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n"
    );
}

#[test]
fn infeasible_input_reports_no_solution_without_writing_csv() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 1));
+choice("c2", bounds(1, 1));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
        ),
    );

    assert!(output.status.success());
    assert!(String::from_utf8_lossy(&output.stdout).is_empty());
}
