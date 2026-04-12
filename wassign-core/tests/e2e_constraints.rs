//! End-to-end tests for each public constraint form.

mod common;

use common::run_cli;

const FORCED_GROUPED_SCHEDULE_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"a\", \"s2\"\n\"b\", \"s1\"\n\"c\", \"s2\"\n\"d\", \"s1\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"b\", \"a\"\n\"p2\", \"b\", \"a\"\n\"p3\", \"d\", \"c\"\n\"p4\", \"d\", \"c\"\n\n";

const MULTIPART_SLOT_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\"x\", \"s1\"\n\"[2] x\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c2\"\n\"p2\", \"x\", \"[2] x\"\n\"p3\", \"x\", \"[2] x\"\n\"p4\", \"c1\", \"c2\"\n\n";

const CHOICE_CHOOSER_EQUALITY_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s1\"\n\"c3\", \"s2\"\n\"c4\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c4\"\n\"p2\", \"c1\", \"c4\"\n\"p3\", \"c2\", \"c3\"\n\"p4\", \"c2\", \"c3\"\n\n";

const CHOICE_CONTAINS_CHOOSER_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s1\"\n\"c3\", \"s2\"\n\"c4\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c4\"\n\"p2\", \"c1\", \"c3\"\n\"p3\", \"c2\", \"c4\"\n\"p4\", \"c2\", \"c3\"\n\n";

const CHOOSER_EQUALITY_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s1\"\n\"c3\", \"s2\"\n\"c4\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"c1\", \"c3\"\n\"p2\", \"c2\", \"c4\"\n\"p3\", \"c2\", \"c4\"\n\"p4\", \"c1\", \"c3\"\n\n";

const PART_INDEXED_CHOOSERS_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"c1\", \"s2\"\n\"c2\", \"s1\"\n\"x\", \"s1\"\n\"[2] x\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"x\", \"[2] x\"\n\"p2\", \"c2\", \"c1\"\n\"p3\", \"x\", \"[2] x\"\n\"p4\", \"c2\", \"c1\"\n\n";

fn run_constraint_case(input: &str) -> common::CliRun {
    run_cli(&["--timeout", "5s"], Some(input))
}

fn assert_constraint_changes_solution(base_input: &str, constrained_input: &str, expected: &str) {
    let baseline = run_constraint_case(base_input);
    let constrained = run_constraint_case(constrained_input);

    baseline.assert_success();
    constrained.assert_success();
    constrained.assert_stdout_exact(expected);
    constrained.assert_stdout_differs_from(&baseline);
}

fn grouped_schedule_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("c", bounds(2, 2));
+choice("d", bounds(2, 2));
+chooser("p1", [100, 90, 0, 0]);
+chooser("p2", [100, 90, 0, 0]);
+chooser("p3", [0, 0, 100, 90]);
+chooser("p4", [0, 0, 100, 90]);
+constraint(choice("c").slot == slot("s2"));
{extra_constraint}
"#
    )
}

fn multipart_schedule_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("x", bounds(2, 2), parts(2));
+chooser("p1", [1, 1, 1]);
+chooser("p2", [1, 1, 1]);
+chooser("p3", [1, 1, 1]);
+chooser("p4", [1, 1, 1]);
+constraint(choice("c1").slot == slot("s1"));
{extra_constraint}
"#
    )
}

fn fixed_schedule_assignment_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 0, 100, 50]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [0, 100, 70, 100]);
+constraint(choice("c1").slot == slot("s1"));
+constraint(choice("c2").slot == slot("s1"));
+constraint(choice("c3").slot == slot("s2"));
+constraint(choice("c4").slot == slot("s2"));
{extra_constraint}
"#
    )
}

fn chooser_equality_input() -> String {
    r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("c3", bounds(2, 2));
+choice("c4", bounds(2, 2));
+chooser("p1", [100, 0, 100, 50]);
+chooser("p2", [100, 70, 100, 70]);
+chooser("p3", [0, 100, 50, 100]);
+chooser("p4", [70, 100, 70, 100]);
+constraint(choice("c1").slot == slot("s1"));
+constraint(choice("c2").slot == slot("s1"));
+constraint(choice("c3").slot == slot("s2"));
+constraint(choice("c4").slot == slot("s2"));
+constraint(chooser("p1").choices == chooser("p4").choices);
"#
    .to_owned()
}

fn multipart_assignment_input(extra_constraint: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(2, 2));
+choice("c2", bounds(2, 2));
+choice("x", bounds(2, 2), parts(2));
+chooser("p1", [100, 90, 40]);
+chooser("p2", [100, 90, 0]);
+chooser("p3", [0, 0, 100]);
+chooser("p4", [60, 50, 80]);
+constraint(choice("c1").slot == slot("s2"));
+constraint(choice("x").slot(1) == slot("s2"));
{extra_constraint}
"#
    )
}

fn exact_two_choices_in_first_slot_input(relation: &str) -> String {
    format!(
        r#"
+slot("s1");
+slot("s2");
+choice("a", bounds(2, 2));
+choice("b", bounds(2, 2));
+choice("c", bounds(2, 2));
+choice("d", bounds(2, 2));
+chooser("p1", [100, 0, 100, 0]);
+chooser("p2", [100, 0, 100, 0]);
+chooser("p3", [0, 100, 0, 100]);
+chooser("p4", [0, 100, 0, 100]);
+constraint(choice("a").slot == slot("s1"));
+constraint(choice("b").slot == slot("s1"));
+constraint(choice("c").slot == slot("s2"));
+constraint(choice("d").slot == slot("s2"));
+constraint(slot("s1").size {relation});
"#
    )
}

const EXACT_TWO_CHOICES_IN_FIRST_SLOT_OUTPUT: &str = "\"Choice\", \"Slot\"\n\"a\", \"s1\"\n\"b\", \"s1\"\n\"c\", \"s2\"\n\"d\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"a\", \"c\"\n\"p2\", \"a\", \"c\"\n\"p3\", \"b\", \"d\"\n\"p4\", \"b\", \"d\"\n\n";

#[test]
fn choice_equals_slot_constraint_changes_the_schedule() {
    let constrained = grouped_schedule_input(r#"+constraint(choice("a").slot == slot("s2"));"#);
    assert_constraint_changes_solution(
        &grouped_schedule_input(""),
        &constrained,
        FORCED_GROUPED_SCHEDULE_OUTPUT,
    );
}

#[test]
fn part_indexed_slot_constraint_changes_the_schedule() {
    let run = run_constraint_case(&multipart_schedule_input(
        r#"+constraint(choice("x").slot(1) == slot("s2"));"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(MULTIPART_SLOT_OUTPUT);
}

#[test]
fn choice_not_equals_slot_constraint_changes_the_schedule() {
    let constrained = grouped_schedule_input(r#"+constraint(choice("a").slot != slot("s1"));"#);
    assert_constraint_changes_solution(
        &grouped_schedule_input(""),
        &constrained,
        FORCED_GROUPED_SCHEDULE_OUTPUT,
    );
}

#[test]
fn choice_equals_choice_slot_constraint_changes_the_schedule() {
    let constrained =
        grouped_schedule_input(r#"+constraint(choice("a").slot == choice("c").slot);"#);
    assert_constraint_changes_solution(
        &grouped_schedule_input(""),
        &constrained,
        FORCED_GROUPED_SCHEDULE_OUTPUT,
    );
}

#[test]
fn choice_not_equals_choice_slot_constraint_changes_the_schedule() {
    let constrained =
        grouped_schedule_input(r#"+constraint(choice("a").slot != choice("d").slot);"#);
    assert_constraint_changes_solution(
        &grouped_schedule_input(""),
        &constrained,
        FORCED_GROUPED_SCHEDULE_OUTPUT,
    );
}

#[test]
fn slot_contains_choice_constraint_changes_the_schedule() {
    let constrained =
        grouped_schedule_input(r#"+constraint(slot("s2").choices.contains(choice("a")));"#);
    assert_constraint_changes_solution(
        &grouped_schedule_input(""),
        &constrained,
        FORCED_GROUPED_SCHEDULE_OUTPUT,
    );
}

#[test]
fn slot_contains_not_choice_constraint_changes_the_schedule() {
    let constrained =
        grouped_schedule_input(r#"+constraint(slot("s1").choices.contains_not(choice("a")));"#);
    assert_constraint_changes_solution(
        &grouped_schedule_input(""),
        &constrained,
        FORCED_GROUPED_SCHEDULE_OUTPUT,
    );
}

#[test]
fn slots_have_same_choices_constraint_changes_the_schedule() {
    let run = run_constraint_case(&grouped_schedule_input(
        r#"+constraint(slot("s1").choices == slot("s2").choices);"#,
    ));

    run.assert_failure();
    run.assert_no_stdout();
    run.assert_stderr_contains("not satisfiable");
}

#[test]
fn slot_size_eq_constraint_accepts_the_matching_boundary() {
    let run = run_constraint_case(&exact_two_choices_in_first_slot_input("== 2"));

    run.assert_success();
    run.assert_stdout_exact(EXACT_TWO_CHOICES_IN_FIRST_SLOT_OUTPUT);
}

#[test]
fn slot_size_neq_constraint_rejects_the_matching_boundary() {
    let run = run_constraint_case(&exact_two_choices_in_first_slot_input("!= 2"));

    run.assert_success();
    run.assert_no_stdout();
}

#[test]
fn slot_size_lt_constraint_rejects_equal_slot_sizes() {
    let run = run_constraint_case(&exact_two_choices_in_first_slot_input("< 2"));

    run.assert_success();
    run.assert_no_stdout();
}

#[test]
fn slot_size_leq_constraint_accepts_equal_slot_sizes() {
    let run = run_constraint_case(&exact_two_choices_in_first_slot_input("<= 2"));

    run.assert_success();
    run.assert_stdout_exact(EXACT_TWO_CHOICES_IN_FIRST_SLOT_OUTPUT);
}

#[test]
fn slot_size_gt_constraint_rejects_equal_slot_sizes() {
    let run = run_constraint_case(&exact_two_choices_in_first_slot_input("> 2"));

    run.assert_success();
    run.assert_no_stdout();
}

#[test]
fn slot_size_geq_constraint_accepts_equal_slot_sizes() {
    let run = run_constraint_case(&exact_two_choices_in_first_slot_input(">= 2"));

    run.assert_success();
    run.assert_stdout_exact(EXACT_TWO_CHOICES_IN_FIRST_SLOT_OUTPUT);
}

#[test]
fn choice_choosers_equality_constraint_changes_the_assignment() {
    let run = run_constraint_case(&fixed_schedule_assignment_input(
        r#"+constraint(choice("c1").choosers == choice("c4").choosers);"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(CHOICE_CHOOSER_EQUALITY_OUTPUT);
}

#[test]
fn choice_contains_chooser_constraint_changes_the_assignment() {
    let run = run_constraint_case(&fixed_schedule_assignment_input(
        r#"+constraint(choice("c4").choosers.contains(chooser("p1")));"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(CHOICE_CONTAINS_CHOOSER_OUTPUT);
}

#[test]
fn part_indexed_choosers_constraint_changes_the_assignment() {
    let run = run_constraint_case(&multipart_assignment_input(
        r#"+constraint(choice("x").choosers(1).contains(chooser("p1")));"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(PART_INDEXED_CHOOSERS_OUTPUT);
}

#[test]
fn choice_contains_not_chooser_constraint_changes_the_assignment() {
    let run = run_constraint_case(&fixed_schedule_assignment_input(
        r#"+constraint(choice("c3").choosers.contains_not(chooser("p1")));"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(CHOICE_CONTAINS_CHOOSER_OUTPUT);
}

#[test]
fn chooser_contains_choice_constraint_changes_the_assignment() {
    let run = run_constraint_case(&fixed_schedule_assignment_input(
        r#"+constraint(chooser("p1").choices.contains(choice("c4")));"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(CHOICE_CONTAINS_CHOOSER_OUTPUT);
}

#[test]
fn chooser_contains_not_choice_constraint_changes_the_assignment() {
    let run = run_constraint_case(&fixed_schedule_assignment_input(
        r#"+constraint(chooser("p1").choices.contains_not(choice("c3")));"#,
    ));

    run.assert_success();
    run.assert_stdout_exact(CHOICE_CONTAINS_CHOOSER_OUTPUT);
}

#[test]
fn choosers_have_same_choices_constraint_changes_the_assignment() {
    let run = run_constraint_case(&chooser_equality_input());

    run.assert_success_clean();
    run.assert_stdout_exact(CHOOSER_EQUALITY_OUTPUT);
}

#[test]
fn unsupported_constraint_shape_is_rejected() {
    let run = run_constraint_case(
        r#"
+slot("s1");
+slot("s2");
+choice("c1", bounds(1, 1));
+choice("c2", bounds(1, 1));
+chooser("p", [0, 0]);
+constraint(choice("c1").slot.contains(choice("c2")));
"#,
    );

    run.assert_failure();
    run.assert_no_stdout();
    run.assert_stderr_contains("Unsupported constraint");
}

#[test]
fn contradictory_constraints_produce_no_solution() {
    let run = run_constraint_case(
        r#"
+slot("s1");
+slot("s2");
+choice("c", bounds(1, 1));
+chooser("p", [0]);
+constraint(choice("c").slot == slot("s1"));
+constraint(slot("s1").choices.contains_not(choice("c")));
"#,
    );

    run.assert_success_clean();
    run.assert_no_stdout();
}

#[test]
fn ambiguous_names_inside_constraints_are_rejected() {
    let run = run_constraint_case(
        r#"
+slot("alpha beta");
+slot("alpha gamma");
+choice("w1", bounds(1, 1));
+chooser("p1", [0]);
+constraint(choice("w1").slot == slot("alpha"));
"#,
    );

    run.assert_failure();
    run.assert_no_stdout();
    run.assert_stderr_contains("ambiguous");
}
