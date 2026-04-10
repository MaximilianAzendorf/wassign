//! End-to-end tests for observable CLI output behavior.

mod common;

use common::{TestDir, run_cli, run_cli_in_dir};

#[test]
fn stdout_contains_only_assignment_csv_for_single_slot_instances() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s");
+choice("e", bounds(1, 1));
+chooser("p", [1]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"s\"\n\"p\", \"e\"\n\n");
    assert!(
        !output.stdout.contains("\"Choice\", \"Slot\""),
        "single-slot output should not include scheduling CSV:\n{}",
        output.stdout
    );
}

#[test]
fn stdout_contains_scheduling_then_assignment_for_multi_slot_instances() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+constraint(choice("c1").slot == slot("s1"));
+constraint(choice("c2").slot == slot("s2"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn output_prefix_writes_both_csv_files_for_multi_slot_instances() {
    let dir = TestDir::new("e2e-cli-output-both-files");
    let prefix = dir.output_prefix("solution");

    let output = run_cli(
        &["--timeout", "1s", "--output", &prefix],
        Some(
            r#"
+slot("east");
+slot("west");
+choice("alpha", bounds(2, 2));
+choice("beta", bounds(2, 2));
+constraint(choice("alpha").slot == slot("east"));
+constraint(choice("beta").slot == slot("west"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
    dir.assert_file_exact(
        "solution.scheduling.csv",
        "\"Choice\", \"Slot\"\n\"alpha\", \"east\"\n\"beta\", \"west\"",
    );
    dir.assert_file_exact(
        "solution.assignment.csv",
        "\"Chooser\", \"east\", \"west\"\n\"p1\", \"alpha\", \"beta\"\n\"p2\", \"alpha\", \"beta\"",
    );
}

#[test]
fn output_prefix_writes_only_assignment_csv_for_single_slot_instances() {
    let dir = TestDir::new("e2e-cli-output-single-file");
    let prefix = dir.output_prefix("solution");

    let output = run_cli(
        &["--timeout", "1s", "--output", &prefix],
        Some(
            r#"
+slot("s");
+choice("e", bounds(1, 1));
+chooser("p", [1]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
    dir.assert_file_exact(
        "solution.assignment.csv",
        "\"Chooser\", \"s\"\n\"p\", \"e\"",
    );
    dir.assert_file_missing("solution.scheduling.csv");
}

#[test]
fn infeasible_instance_produces_no_stdout_csv() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("only");
+choice("alpha", bounds(1, 1));
+choice("beta", bounds(1, 1));
+chooser("p1", [0, 1]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
}

#[test]
fn infeasible_instance_with_output_prefix_does_not_write_solution_files() {
    let dir = TestDir::new("e2e-cli-output-infeasible");
    let prefix = dir.output_prefix("solution");

    let output = run_cli(
        &["--timeout", "1s", "--output", &prefix],
        Some(
            r#"
+slot("only");
+choice("alpha", bounds(1, 1));
+choice("beta", bounds(1, 1));
+chooser("p1", [0, 1]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
    dir.assert_file_missing("solution.assignment.csv");
    dir.assert_file_missing("solution.scheduling.csv");
}

#[test]
fn input_parse_failure_is_reported_on_stderr() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("only");
+choice("task", bounds(1, 1))
+chooser("p1", [0]);
"#,
        ),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
}

#[test]
fn unsatisfiable_constraints_are_reported_as_input_errors() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 1));
+chooser("p1", [0]);
+constraint(slot("s1").choices == slot("s2").choices);
"#,
        ),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("not satisfiable");
}

#[test]
fn missing_input_file_failure_is_reported_on_stderr() {
    let dir = TestDir::new("e2e-cli-output-missing-input");

    let output = run_cli_in_dir(&dir, &["--input", "missing-file.wassign"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("No such file");
}

#[test]
fn generated_part_names_are_hidden_in_csv_output() {
    let output = run_cli(
        &["--timeout", "5s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2), parts(2));
+constraint(choice("c1").slot == slot("s2"));
+constraint(choice("c2").slot == slot("s1"));
+chooser("p1", [100, 0, 100]);
+chooser("p2", [100, 0, 100]);
+chooser("p3", [0, 100, 100]);
+chooser("p4", [0, 100, 100]);
"#,
        ),
    );

    output.assert_success_clean();
    let parsed = output.parse_stdout_csv();
    let scheduling = parsed
        .scheduling
        .expect("multipart output should include scheduling CSV");
    let assignment = parsed
        .assignment
        .expect("multipart output should include assignment CSV");

    assert_eq!(scheduling.header, vec!["Choice", "Slot"]);
    scheduling.assert_row_set(&[
        vec!["c1", "s2"],
        vec!["c2", "s1"],
        vec!["c3", "s1"],
        vec!["[2] c3", "s2"],
    ]);
    assert_eq!(assignment.header, vec!["Chooser", "s1", "s2"]);
    assignment.assert_row_set(&[
        vec!["p1", "c2", "c1"],
        vec!["p2", "c2", "c1"],
        vec!["p3", "c3", "[2] c3"],
        vec!["p4", "c3", "[2] c3"],
    ]);
    assert!(
        !output.stdout.contains("\"~[2] c3\""),
        "internal multipart names leaked into stdout:\n{}",
        output.stdout
    );
}

#[test]
fn output_write_failure_is_reported_as_an_error() {
    let dir = TestDir::new("e2e-cli-output-write-failure");
    let prefix = dir.path("missing/solution").to_string_lossy().into_owned();

    let output = run_cli(
        &["--timeout", "1s", "--output", &prefix],
        Some(
            r#"
+slot("east");
+slot("west");
+choice("alpha", bounds(2, 2));
+choice("beta", bounds(2, 2));
+constraint(choice("alpha").slot == slot("east"));
+constraint(choice("beta").slot == slot("west"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
        ),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("No such file");
}
