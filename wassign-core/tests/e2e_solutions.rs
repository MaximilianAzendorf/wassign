//! End-to-end tests for exact solution production.

mod common;

use std::collections::BTreeSet;

use common::{TestDir, run_cli, run_cli_in_dir};

fn forced_two_slot_instance() -> &'static str {
    r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0]);
+chooser("p2", [0, 1]);
"#
}

fn forced_two_slot_output() -> &'static str {
    "\"Choice\", \"Slot\"\n\
\"a\", \"s1\"\n\
\"b\", \"s2\"\n\n\
\"Chooser\", \"s1\", \"s2\"\n\
\"p1\", \"a\", \"b\"\n\
\"p2\", \"a\", \"b\"\n\n"
}

#[test]
fn minimal_instance_has_the_exact_expected_output() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(1, 1));
+choice("b", bounds(1, 1));
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
\"a\", \"s1\"\n\
\"b\", \"s2\"\n\n\
\"Chooser\", \"s1\", \"s2\"\n\
\"p1\", \"a\", \"b\"\n\n",
    );
}

#[test]
fn two_slot_two_choice_instance_has_a_unique_exact_solution() {
    let output = run_cli(&["--timeout", "1s"], Some(forced_two_slot_instance()));

    output.assert_success();
    output.assert_stdout_exact(forced_two_slot_output());
}

#[test]
fn unconstrained_multi_slot_instance_still_has_a_unique_optimum() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(1, 2));
+choice("b", bounds(1, 2));
+choice("c", bounds(1, 2));
+chooser("p1", [3, 0, 2]);
+chooser("p2", [0, 3, 2]);
"#,
        ),
    );

    output.assert_success();

    let expected_first = "\"Choice\", \"Slot\"\n\
\"a\", \"s1\"\n\
\"b\", \"s1\"\n\
\"c\", \"s2\"\n\n\
\"Chooser\", \"s1\", \"s2\"\n\
\"p1\", \"a\", \"c\"\n\
\"p2\", \"b\", \"c\"\n\n";
    let expected_second = "\"Choice\", \"Slot\"\n\
\"a\", \"s2\"\n\
\"b\", \"s2\"\n\
\"c\", \"s1\"\n\n\
\"Chooser\", \"s1\", \"s2\"\n\
\"p1\", \"c\", \"a\"\n\
\"p2\", \"c\", \"b\"\n\n";

    assert!(
        output.stdout == expected_first || output.stdout == expected_second,
        "unexpected stdout:\n{}",
        output.stdout
    );
}

#[test]
fn multipart_choice_instance_has_the_exact_expected_output() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+slot("s3");
+slot("s4");
+choice("intro", bounds(2, 2));
+choice("deep", bounds(2, 2), parts(2));
+choice("outro", bounds(2, 2));
+constraint(choice("intro").slot == slot("s1"));
+constraint(choice("outro").slot == slot("s4"));
+chooser("p1", [3, 2, 1]);
+chooser("p2", [3, 2, 1]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
\"intro\", \"s1\"\n\
\"deep\", \"s2\"\n\
\"[2] deep\", \"s3\"\n\
\"outro\", \"s4\"\n\n\
\"Chooser\", \"s1\", \"s2\", \"s3\", \"s4\"\n\
\"p1\", \"intro\", \"deep\", \"[2] deep\", \"outro\"\n\
\"p2\", \"intro\", \"deep\", \"[2] deep\", \"outro\"\n\n",
    );
}

#[test]
fn optional_choice_can_be_left_unscheduled_in_the_optimal_solution() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("x", bounds(2, 2), optional);
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0, 0]);
+chooser("p2", [0, 1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
\"a\", \"s1\"\n\
\"b\", \"s2\"\n\
\"x\", \"not scheduled\"\n\n\
\"Chooser\", \"s1\", \"s2\"\n\
\"p1\", \"a\", \"b\"\n\
\"p2\", \"a\", \"b\"\n\n",
    );
}

#[test]
fn optional_keyword_matches_optional_if_true() {
    let with_keyword = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("x", bounds(2, 2), optional);
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0, 0]);
+chooser("p2", [0, 1, 0]);
"#,
        ),
    );
    let with_conditional = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("x", bounds(2, 2), optional_if(true));
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0, 0]);
+chooser("p2", [0, 1, 0]);
"#,
        ),
    );

    with_keyword.assert_success();
    with_conditional.assert_success();
    assert_eq!(with_keyword.stdout, with_conditional.stdout);
}

#[test]
fn optional_if_true_matches_optional_behavior() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("x", bounds(2, 2), optional_if(true));
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0, 0]);
+chooser("p2", [0, 1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
\"a\", \"s1\"\n\
\"b\", \"s2\"\n\
\"x\", \"not scheduled\"\n\n\
\"Chooser\", \"s1\", \"s2\"\n\
\"p1\", \"a\", \"b\"\n\
\"p2\", \"a\", \"b\"\n\n",
    );
}

#[test]
fn optional_if_false_keeps_the_choice_required() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("x", bounds(2, 2), optional_if(false));
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0, 0]);
+chooser("p2", [0, 1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
}

#[test]
fn single_thread_and_multi_thread_runs_produce_the_same_exact_output_on_a_stable_instance() {
    let single_thread = run_cli(
        &["--timeout", "1s", "--threads", "1"],
        Some(forced_two_slot_instance()),
    );
    let multi_thread = run_cli(
        &["--timeout", "1s", "--threads", "4"],
        Some(forced_two_slot_instance()),
    );

    single_thread.assert_success();
    multi_thread.assert_success();
    single_thread.assert_stdout_exact(forced_two_slot_output());
    assert_eq!(single_thread.stdout, multi_thread.stdout);
}

#[test]
fn any_mode_still_returns_a_valid_public_output_shape() {
    let output = run_cli(
        &["--timeout", "1s", "--any"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(1, 2));
+choice("b", bounds(1, 2));
+chooser("p1", [1, 1]);
+chooser("p2", [1, 1]);
"#,
        ),
    );

    output.assert_success();

    let sections = output.stdout.split("\n\n").collect::<Vec<_>>();
    assert_eq!(sections.len(), 3, "unexpected stdout:\n{}", output.stdout);

    let scheduling_lines = sections[0].lines().collect::<Vec<_>>();
    assert_eq!(scheduling_lines[0], "\"Choice\", \"Slot\"");
    assert_eq!(scheduling_lines.len(), 3);
    let scheduling_rows = scheduling_lines[1..]
        .iter()
        .map(|line| (*line).to_owned())
        .collect::<BTreeSet<String>>();
    assert!(
        scheduling_rows
            == BTreeSet::from(["\"a\", \"s1\"".to_owned(), "\"b\", \"s2\"".to_owned(),])
            || scheduling_rows
                == BTreeSet::from(["\"a\", \"s2\"".to_owned(), "\"b\", \"s1\"".to_owned(),]),
        "unexpected scheduling rows: {scheduling_rows:?}"
    );

    let assignment_lines = sections[1].lines().collect::<Vec<_>>();
    assert_eq!(assignment_lines[0], "\"Chooser\", \"s1\", \"s2\"");
    assert_eq!(assignment_lines.len(), 3);
    let assignment_rows = assignment_lines[1..]
        .iter()
        .map(|line| (*line).to_owned())
        .collect::<BTreeSet<String>>();
    assert_eq!(
        assignment_rows.len(),
        2,
        "unexpected assignment rows: {assignment_rows:?}"
    );
    for chooser in ["p1", "p2"] {
        let first = format!("\"{chooser}\", \"a\", \"b\"");
        let second = format!("\"{chooser}\", \"b\", \"a\"");
        assert!(
            assignment_rows.contains(&first) || assignment_rows.contains(&second),
            "missing assignment row for {chooser}: {assignment_rows:?}"
        );
    }
}

#[test]
fn infeasible_capacity_mismatch_produces_no_solution() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+chooser("p1", [0, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
}

#[test]
fn infeasible_constraint_system_produces_no_solution() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(1, 1));
+choice("b", bounds(1, 1));
+constraint(choice("a").slot == choice("b").slot);
+chooser("p1", [0, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
}

#[test]
fn low_timeout_exits_cleanly_on_a_trivial_instance() {
    let output = run_cli(
        &["--timeout", "0s"],
        Some(
            r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(1, 1));
+choice("b", bounds(1, 1));
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_no_stdout();
}

#[test]
fn split_input_across_files_produces_the_same_exact_solution_as_single_file_input() {
    let dir = TestDir::new("e2e-solutions-split-input");
    dir.write(
        "part1.wassign",
        r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+constraint(choice("a").slot == slot("s1"));
"#,
    );
    dir.write(
        "part2.wassign",
        r#"
+constraint(choice("b").slot == slot("s2"));
+chooser("p1", [1, 0]);
+chooser("p2", [0, 1]);
"#,
    );

    let single_file = run_cli(&["--timeout", "1s"], Some(forced_two_slot_instance()));
    let split_files = run_cli_in_dir(
        &dir,
        &[
            "--timeout",
            "1s",
            "--input",
            "part1.wassign",
            "--input",
            "part2.wassign",
        ],
        None,
    );

    single_file.assert_success();
    split_files.assert_success();
    single_file.assert_stdout_exact(forced_two_slot_output());
    assert_eq!(single_file.stdout, split_files.stdout);
}
