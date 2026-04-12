//! End-to-end tests for CLI argument behavior.

mod common;

use common::{TestDir, run_cli, run_cli_in_dir};

const MINIMAL_STDIN_INPUT: &str = r#"
+slot("s");
+choice("e", bounds(1, 1));
+chooser("p", [1]);
"#;

const DETERMINISTIC_TWO_SLOT_INPUT: &str = r#"
+slot("s1");
+slot("s2");
let c1 = +choice("c1", bounds(2, 2));
let c2 = +choice("c2", bounds(2, 2));
+constraint(c1.slot == slot("s1"));
+constraint(c2.slot == slot("s2"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#;

#[test]
fn stdin_is_used_when_no_input_flag_is_given() {
    let output = run_cli(&["--timeout", "1s"], Some(MINIMAL_STDIN_INPUT));

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"s\"\n\"p\", \"e\"\n\n");
}

#[test]
fn single_input_file_is_read() {
    let dir = TestDir::new("e2e-cli-args-single-input");
    dir.write("problem.wassign", DETERMINISTIC_TWO_SLOT_INPUT);

    let output = run_cli_in_dir(
        &dir,
        &["--input", "problem.wassign", "--timeout", "1s"],
        None,
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn multiple_input_files_are_concatenated_in_argument_order() {
    let dir = TestDir::new("e2e-cli-args-multi-input");
    dir.write(
        "01-base.wassign",
        r#"
+slot("s1");
+slot("s2");
let c1 = +choice("c1", bounds(2, 2));
let c2 = +choice("c2", bounds(2, 2));
"#,
    );
    dir.write(
        "02-constraints.wassign",
        r#"
+constraint(c1.slot == slot("s1"));
+constraint(c2.slot == slot("s2"));
+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
    );

    let output = run_cli_in_dir(
        &dir,
        &[
            "--input",
            "01-base.wassign",
            "--input",
            "02-constraints.wassign",
            "--timeout",
            "1s",
        ],
        None,
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn missing_input_file_is_reported_as_an_error() {
    let dir = TestDir::new("e2e-cli-args-missing-input");

    let output = run_cli_in_dir(
        &dir,
        &["--input", "missing.wassign", "--timeout", "1s"],
        None,
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input");
    output.assert_stderr_contains("No such file");
}

#[test]
fn unreadable_input_file_is_reported_as_an_error() {
    let dir = TestDir::new("e2e-cli-args-unreadable-input");
    std::fs::create_dir(dir.path("not-a-file")).expect("directory fixture should be creatable");

    let output = run_cli_in_dir(&dir, &["--input", "not-a-file", "--timeout", "1s"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input");
    output.assert_stderr_contains("directory");
}

#[test]
fn invalid_timeout_string_is_rejected() {
    let output = run_cli(&["--timeout", "1x"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("--timeout");
    output.assert_stderr_contains("Unknown time specifier 1x");
}

#[test]
fn compound_timeout_string_is_accepted() {
    let output = run_cli(&["--timeout", "1d30m"], Some(MINIMAL_STDIN_INPUT));

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"s\"\n\"p\", \"e\"\n\n");
}

#[test]
fn invalid_critical_set_timeout_string_is_rejected() {
    let output = run_cli(
        &["--timeout", "1s", "--cs-timeout", "1x"],
        Some(MINIMAL_STDIN_INPUT),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("--cs-timeout");
    output.assert_stderr_contains("Unknown time specifier 1x");
}

#[test]
fn zero_timeout_still_exits_cleanly() {
    let output = run_cli(&["--timeout", "0s"], Some(MINIMAL_STDIN_INPUT));

    output.assert_success();
    output.assert_no_stdout();
}

#[test]
fn invalid_thread_count_value_is_rejected() {
    let output = run_cli(
        &["--threads", "nope", "--timeout", "1s"],
        Some(MINIMAL_STDIN_INPUT),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("--threads");
    output.assert_stderr_contains("invalid value");
}

#[test]
fn invalid_preference_exponent_value_is_rejected() {
    let output = run_cli(
        &["--pref-exp", "nope", "--timeout", "1s"],
        Some(MINIMAL_STDIN_INPUT),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("--pref-exp");
    output.assert_stderr_contains("invalid value");
}

#[test]
fn invalid_max_neighbors_value_is_rejected() {
    let output = run_cli(
        &["--max-neighbors", "nope", "--timeout", "1s"],
        Some(MINIMAL_STDIN_INPUT),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("--max-neighbors");
    output.assert_stderr_contains("invalid value");
}

#[test]
fn greedy_mode_is_accepted_and_solves_a_deterministic_instance() {
    let output = run_cli(
        &["--greedy", "--timeout", "1s"],
        Some(DETERMINISTIC_TWO_SLOT_INPUT),
    );

    output.assert_success_clean();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn no_critical_sets_flag_is_accepted_and_preserves_correctness_on_a_deterministic_instance() {
    let output = run_cli(
        &["--no-cs", "--timeout", "1s"],
        Some(DETERMINISTIC_TWO_SLOT_INPUT),
    );

    output.assert_success_clean();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn no_critical_set_simplification_flag_is_accepted_and_preserves_correctness() {
    let output = run_cli(
        &["--no-cs-simp", "--timeout", "1s"],
        Some(DETERMINISTIC_TWO_SLOT_INPUT),
    );

    output.assert_success_clean();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn single_thread_mode_preserves_the_public_solution_on_a_stable_instance() {
    let output = run_cli(
        &["--threads", "1", "--timeout", "1s"],
        Some(DETERMINISTIC_TWO_SLOT_INPUT),
    );

    output.assert_success_clean();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn low_max_neighbors_flag_is_accepted_and_still_solves_a_deterministic_instance() {
    let output = run_cli(
        &["--max-neighbors", "1", "--timeout", "1s"],
        Some(DETERMINISTIC_TWO_SLOT_INPUT),
    );

    output.assert_success_clean();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \
         \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"c1\", \"c2\"\n\n",
    );
}
