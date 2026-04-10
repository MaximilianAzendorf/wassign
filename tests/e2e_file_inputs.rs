//! End-to-end tests for file-backed input helpers.

mod common;

use common::{TestDir, run_cli_in_dir};

#[test]
fn read_csv_with_default_separator_can_build_a_complete_problem() {
    let dir = TestDir::new("e2e-file-inputs-default-csv");
    dir.write("slots.csv", "\"Slot\"\n\"s1\"\n\"s2\"\n");
    dir.write(
        "choices.csv",
        "\"Choice\",\"Slot\"\n\"alpha\",\"s1\"\n\"beta\",\"s2\"\n",
    );
    dir.write(
        "choosers.csv",
        "\"Chooser\",\"alpha\",\"beta\"\n\"p1\",0,1\n\"p2\",1,0\n",
    );
    dir.write(
        "input.wassign",
        r#"
let slots = read_csv("slots.csv");
for row in slots.rows.slice(1, end) {
    +slot(row[0]);
}

let choices = read_csv("choices.csv");
for row in choices.rows.slice(1, end) {
    let created = +choice(row[0], bounds(2, 2));
    +constraint(created.slot == slot(row[1]));
}

let choosers = read_csv("choosers.csv");
for row in choosers.rows.slice(1, end) {
    +chooser(row[0], [row[1], row[2]]);
}
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"alpha\", \"s1\"\n\
         \"beta\", \"s2\"\n\n\
         \"Chooser\", \"s1\", \"s2\"\n\
         \"p1\", \"alpha\", \"beta\"\n\
         \"p2\", \"alpha\", \"beta\"\n\n",
    );
}

#[test]
fn read_csv_with_custom_separator_can_build_a_complete_problem() {
    let dir = TestDir::new("e2e-file-inputs-custom-csv");
    dir.write("slots.csv", "Slot\ns1\ns2\n");
    dir.write("choices.csv", "Choice;Slot\nalpha;s1\nbeta;s2\n");
    dir.write("choosers.csv", "Chooser;alpha;beta\np1;0;1\np2;1;0\n");
    dir.write(
        "input.wassign",
        r#"
let slots = read_csv("slots.csv");
for row in slots.rows.slice(1, end) {
    +slot(row[0]);
}

let choices = read_csv("choices.csv", ";");
for row in choices.rows.slice(1, end) {
    let created = +choice(row[0], bounds(2, 2));
    +constraint(created.slot == slot(row[1]));
}

let choosers = read_csv("choosers.csv", ';');
for row in choosers.rows.slice(1, end) {
    +chooser(row[0], [row[1], row[2]]);
}
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"alpha\", \"s1\"\n\
         \"beta\", \"s2\"\n\n\
         \"Chooser\", \"s1\", \"s2\"\n\
         \"p1\", \"alpha\", \"beta\"\n\
         \"p2\", \"alpha\", \"beta\"\n\n",
    );
}

#[test]
fn csv_row_accessor_can_select_specific_rows() {
    let dir = TestDir::new("e2e-file-inputs-row");
    dir.write(
        "choices.csv",
        "\"Choice\",\"Slot\"\n\"alpha\",\"s1\"\n\"beta\",\"s2\"\n\"gamma\",\"s3\"\n",
    );
    dir.write(
        "input.wassign",
        r#"
+slot("s1");
+slot("s2");
+slot("s3");

let file = read_csv("choices.csv");
let first = file.row(1);
let picked = file.row(2);
let last = file.row(3);

+choice(first[0], bounds(1, 1));
+choice(picked[0], bounds(1, 1));
+choice(last[0], bounds(1, 1));

+constraint(choice(first[0]).slot == slot(first[1]));
+constraint(choice(picked[0]).slot == slot(picked[1]));
+constraint(choice(last[0]).slot == slot(last[1]));

+chooser("p", [0, 1, 2]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"alpha\", \"s1\"\n\
         \"beta\", \"s2\"\n\
         \"gamma\", \"s3\"\n\n\
         \"Chooser\", \"s1\", \"s2\", \"s3\"\n\
         \"p\", \"alpha\", \"beta\", \"gamma\"\n\n",
    );
}

#[test]
fn csv_index_accessor_can_select_specific_rows() {
    let dir = TestDir::new("e2e-file-inputs-index");
    dir.write(
        "choices.csv",
        "\"Choice\",\"Slot\"\n\"alpha\",\"s1\"\n\"beta\",\"s2\"\n\"gamma\",\"s3\"\n",
    );
    dir.write(
        "input.wassign",
        r#"
+slot("s1");
+slot("s2");
+slot("s3");

let file = read_csv("choices.csv");
let first = file[1];
let picked = file[2];
let last = file[3];

+choice(first[0], bounds(1, 1));
+choice(picked[0], bounds(1, 1));
+choice(last[0], bounds(1, 1));

+constraint(choice(first[0]).slot == slot(first[1]));
+constraint(choice(picked[0]).slot == slot(picked[1]));
+constraint(choice(last[0]).slot == slot(last[1]));

+chooser("p", [0, 1, 2]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"alpha\", \"s1\"\n\
         \"beta\", \"s2\"\n\
         \"gamma\", \"s3\"\n\n\
         \"Chooser\", \"s1\", \"s2\", \"s3\"\n\
         \"p\", \"alpha\", \"beta\", \"gamma\"\n\n",
    );
}

#[test]
fn csv_rows_slice_with_end_can_select_a_tail_range() {
    let dir = TestDir::new("e2e-file-inputs-slice");
    dir.write(
        "choices.csv",
        "\"Choice\",\"Slot\"\n\"alpha\",\"s1\"\n\"beta\",\"s2\"\n",
    );
    dir.write(
        "input.wassign",
        r#"
+slot("s1");
+slot("s2");

let file = read_csv("choices.csv");
for row in file.rows.slice(1, end) {
    let created = +choice(row[0], bounds(2, 2));
    +constraint(created.slot == slot(row[1]));
}

+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"alpha\", \"s1\"\n\
         \"beta\", \"s2\"\n\n\
         \"Chooser\", \"s1\", \"s2\"\n\
         \"p1\", \"alpha\", \"beta\"\n\
         \"p2\", \"alpha\", \"beta\"\n\n",
    );
}

#[test]
fn csv_numeric_strings_are_accepted_by_choice_helpers() {
    let dir = TestDir::new("e2e-file-inputs-choice-helpers");
    dir.write(
        "choices.csv",
        "\"Choice\",\"minimum\",\"maximum\",\"parts\",\"left\",\"right\"\n\
         \"relay\",2,2,2,\"s1\",\"s2\"\n",
    );
    dir.write(
        "input.wassign",
        r#"
let row = read_csv("choices.csv").row(1);

+slot(row[4]);
+slot(row[5]);

let relay = +choice(row[0], bounds(row[1], row[2]), parts(row[3]));
let snack1 = +choice("snack1", bounds(0, 2));
let snack2 = +choice("snack2", bounds(0, 2));

+constraint(relay.slot(0) == slot(row[4]));
+constraint(relay.slot(1) == slot(row[5]));
+constraint(snack1.slot == slot(row[4]));
+constraint(snack2.slot == slot(row[5]));

+chooser("p1", [0, 9, 9]);
+chooser("p2", [9, 0, 0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"relay\", \"s1\"\n\
         \"[2] relay\", \"s2\"\n\
         \"snack1\", \"s1\"\n\
         \"snack2\", \"s2\"\n\n\
         \"Chooser\", \"s1\", \"s2\"\n\
         \"p1\", \"relay\", \"[2] relay\"\n\
         \"p2\", \"relay\", \"[2] relay\"\n\n",
    );
}

#[test]
fn missing_csv_file_is_reported_as_an_error() {
    let dir = TestDir::new("e2e-file-inputs-missing-csv");
    dir.write(
        "input.wassign",
        r#"
let file = read_csv("missing.csv");
let row = file.row(0);

+slot("s");
+choice(row[0], bounds(1, 1));
+chooser("p", [0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("No such file or directory");
}

#[test]
fn out_of_range_csv_row_access_is_rejected() {
    let dir = TestDir::new("e2e-file-inputs-row-oob");
    dir.write("choices.csv", "\"Choice\"\n\"alpha\"\n");
    dir.write(
        "input.wassign",
        r#"
let file = read_csv("choices.csv");
let row = file.row(9);

+slot("s");
+choice(row[0], bounds(1, 1));
+chooser("p", [0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("CSV row 9 is out of bounds.");
}

#[test]
fn read_file_can_supply_names_or_script_fragments() {
    let dir = TestDir::new("e2e-file-inputs-read-file");
    dir.write("slot-name.txt", "late slot");
    dir.write(
        "input.wassign",
        r#"
let late = readFile("slot-name.txt");

+slot("early");
+slot(late);

let opening = +choice("opening", bounds(2, 2));
let talk = +choice("talk", bounds(2, 2));
+constraint(opening.slot == slot("early"));
+constraint(talk.slot == slot(late));

+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"opening\", \"early\"\n\
         \"talk\", \"late slot\"\n\n\
         \"Chooser\", \"early\", \"late slot\"\n\
         \"p1\", \"opening\", \"talk\"\n\
         \"p2\", \"opening\", \"talk\"\n\n",
    );
}

#[test]
fn missing_read_file_target_is_reported_as_an_error() {
    let dir = TestDir::new("e2e-file-inputs-missing-read-file");
    dir.write(
        "input.wassign",
        r#"
let late = readFile("missing.txt");

+slot("s");
+choice("talk", bounds(1, 1));
+chooser("p", [0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("No such file or directory");
}

#[test]
fn set_arguments_can_override_timeout_inside_input() {
    let dir = TestDir::new("e2e-file-inputs-set-timeout");
    dir.write(
        "input.wassign",
        r#"
set_arguments(["--timeout", "1s"]);

+slot("s1");
+slot("s2");

let alpha = +choice("alpha", bounds(2, 2));
let beta = +choice("beta", bounds(2, 2));

+constraint(alpha.slot == slot("s1"));
+constraint(beta.slot == slot("s2"));

+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
    );

    let output = run_cli_in_dir(
        &dir,
        &["--input", "input.wassign", "--timeout", "0s", "--no-cs"],
        None,
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\
         \"alpha\", \"s1\"\n\
         \"beta\", \"s2\"\n\n\
         \"Chooser\", \"s1\", \"s2\"\n\
         \"p1\", \"alpha\", \"beta\"\n\
         \"p2\", \"alpha\", \"beta\"\n\n",
    );
}

#[test]
fn set_arguments_can_enable_greedy_mode_inside_input() {
    let dir = TestDir::new("e2e-file-inputs-set-greedy");
    dir.write(
        "input.wassign",
        r#"
set_arguments(["--greedy"]);

+slot("s1");
+slot("s2");
+choice("a", bounds(1, 2));
+choice("b", bounds(1, 2));
+choice("c", bounds(1, 2));
+chooser("p1", [0, 0, 0]);
+chooser("p2", [0, 0, 1]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_success_clean();
}

#[test]
fn invalid_set_arguments_input_is_rejected() {
    let dir = TestDir::new("e2e-file-inputs-invalid-set-args");
    dir.write(
        "input.wassign",
        r#"
set_arguments(["--timeout"]);

+slot("s");
+choice("alpha", bounds(1, 1));
+chooser("p", [0]);
"#,
    );

    let output = run_cli_in_dir(&dir, &["--input", "input.wassign", "--timeout", "1s"], None);

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Error in input:");
    output.assert_stderr_contains("Missing value for --timeout.");
}

#[test]
fn cli_arguments_and_set_arguments_have_a_defined_precedence() {
    let dir = TestDir::new("e2e-file-inputs-arg-precedence");
    dir.write(
        "input.wassign",
        r#"
set_arguments(["--output", "script-output"]);

+slot("s1");
+slot("s2");

let alpha = +choice("alpha", bounds(2, 2));
let beta = +choice("beta", bounds(2, 2));

+constraint(alpha.slot == slot("s1"));
+constraint(beta.slot == slot("s2"));

+chooser("p1", [0, 1]);
+chooser("p2", [1, 0]);
"#,
    );

    let output = run_cli_in_dir(
        &dir,
        &[
            "--input",
            "input.wassign",
            "--output",
            "cli-output",
            "--timeout",
            "1s",
        ],
        None,
    );

    output.assert_success();
    output.assert_no_stdout();
    dir.assert_file_exact(
        "script-output.scheduling.csv",
        "\"Choice\", \"Slot\"\n\"alpha\", \"s1\"\n\"beta\", \"s2\"",
    );
    dir.assert_file_exact(
        "script-output.assignment.csv",
        "\"Chooser\", \"s1\", \"s2\"\n\"p1\", \"alpha\", \"beta\"\n\"p2\", \"alpha\", \"beta\"",
    );
    dir.assert_file_missing("cli-output.scheduling.csv");
    dir.assert_file_missing("cli-output.assignment.csv");
}
