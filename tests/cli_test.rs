//! Integration tests for the Rust CLI surface.

mod common;

use common::{run_cli, temp_file_path, write_temp_file};

const WASSIGN_VERSION: &str = env!("CARGO_PKG_VERSION");

#[test]
fn help_lists_supported_flags_without_verbosity() {
    let output = run_cli(&["--help"], None);

    output.assert_success();
    let stdout = &output.stdout;
    assert!(stdout.contains("Usage: wassign"));
    assert!(stdout.contains("--max-neighbors"));
    assert!(stdout.contains("--no-cs-simp"));
    assert!(!stdout.contains("--verbosity"));
    assert!(!stdout.contains(" -v"));
}

#[test]
fn version_reports_the_binary_version() {
    let output = run_cli(&["--version"], None);

    output.assert_success();
    assert_eq!(output.stdout.trim(), format!("wassign {WASSIGN_VERSION}"));
}

#[test]
fn legacy_verbosity_flag_is_rejected() {
    let output = run_cli(&["-v"], None);

    output.assert_failure();
    let stderr = &output.stderr;
    assert!(stderr.contains("-v"));
    assert!(stderr.contains("unexpected argument"));
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
        &[
            "--input",
            &input_path.to_string_lossy(),
            "--output",
            &output_prefix,
            "--timeout",
            "1s",
        ],
        None,
    );

    output.assert_success();

    let scheduling_path = format!("{output_prefix}.scheduling.csv");
    let assignment_path = format!("{output_prefix}.assignment.csv");
    let scheduling =
        std::fs::read_to_string(&scheduling_path).expect("scheduling file should exist");
    let assignment =
        std::fs::read_to_string(&assignment_path).expect("assignment file should exist");

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

    output.assert_success();
    assert_eq!(
        output.stdout,
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

    output.assert_success();
    output.assert_no_stdout();
}
