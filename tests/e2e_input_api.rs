//! End-to-end tests for the public Rhai input API.

mod common;

use common::run_cli;

#[test]
fn add_function_registers_objects_the_same_as_unary_plus() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let s1 = slot("s1");
let s2 = slot("s2");
add(s1);
add(s2);

let c1 = choice("c1", bounds(1, 1));
let c2 = choice("c2", bounds(1, 1));
add(c1);
add(c2);

let p = chooser("p", [0, 1]);
add(p);

add(constraint(c1.slot == s1));
add(constraint(c2.slot == s2));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn unregistered_new_object_is_rejected() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+choice("c1", bounds(1, 1));
+chooser("p", [0]);

slot("dangling");
"#,
        ),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Newly created object not used");
}

#[test]
fn exact_name_lookup_reuses_existing_objects() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("Morning");
+slot("Afternoon");
+choice("Main Talk", bounds(1, 1));
+choice("Workshop", bounds(1, 1));
+chooser("Ada", [0, 1]);

+constraint(choice("Main Talk").slot == slot("Morning"));
+constraint(choice("Workshop").slot == slot("Afternoon"));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"Main Talk\", \"Morning\"\n\"Workshop\", \"Afternoon\"\n\n\"Chooser\", \"Morning\", \"Afternoon\"\n\"Ada\", \"Main Talk\", \"Workshop\"\n\n",
    );
}

#[test]
fn fuzzy_name_lookup_by_token_is_supported() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("Room Red");
+slot("Room Blue");
+choice("Graph Theory", bounds(1, 1));
+choice("Compiler Systems", bounds(1, 1));
+chooser("p", [0, 1]);

+constraint(choice("Theory").slot == slot("Blue"));
+constraint(choice("Compiler Systems").slot == slot("Room Red"));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"Graph Theory\", \"Room Blue\"\n\"Compiler Systems\", \"Room Red\"\n\n\"Chooser\", \"Room Red\", \"Room Blue\"\n\"p\", \"Compiler Systems\", \"Graph Theory\"\n\n",
    );
}

#[test]
fn ambiguous_fuzzy_name_lookup_is_rejected() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("alpha beta");
+slot("alpha gamma");
+choice("w1", bounds(1, 1));
+chooser("p1", [0]);
+constraint(choice("w1").slot == slot("alpha"));
"#,
        ),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("The name \"alpha\" is ambiguous.");
}

#[test]
fn set_name_can_rename_slots_choices_and_choosers() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let first_slot = +slot("slot-before");
let second_slot = +slot("slot-after");
let first_choice = +choice("choice-before", bounds(1, 1));
let second_choice = +choice("choice-after", bounds(1, 1));
let attendee = +chooser("chooser-before", [0, 1]);

set_name(first_slot, "Morning Session!");
set_name(first_choice, "Renamed Choice, A");
set_name(attendee, "Dr. Ada, Jr.");

+constraint(choice("Renamed Choice, A").slot == slot("Morning Session!"));
+constraint(choice("choice-after").slot == slot("slot-after"));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"Renamed Choice, A\", \"Morning Session!\"\n\"choice-after\", \"slot-after\"\n\n\"Chooser\", \"Morning Session!\", \"slot-after\"\n\"Dr. Ada, Jr.\", \"Renamed Choice, A\", \"choice-after\"\n\n",
    );
}

#[test]
fn duplicate_names_after_renaming_are_rejected() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let s1 = +slot("s1");
let s2 = +slot("s2");
+set_name(s2, "s1");

+choice("c1", bounds(1, 1));
+chooser("p", [0]);
"#,
        ),
    );

    output.assert_failure();
    output.assert_no_stdout();
    output.assert_stderr_contains("Duplicate set name 's1'");
}

#[test]
fn names_with_spaces_and_punctuation_are_supported() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("Room, A!");
+slot("Room: B?");
+choice("Kid's Choice, Alpha", bounds(1, 1));
+choice("Beta/2026", bounds(1, 1));
+chooser("Dr. Q, Jr.", [0, 1]);

+constraint(choice("Kid's Choice, Alpha").slot == slot("Room, A!"));
+constraint(choice("Beta/2026").slot == slot("Room: B?"));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"Kid's Choice, Alpha\", \"Room, A!\"\n\"Beta/2026\", \"Room: B?\"\n\n\"Chooser\", \"Room, A!\", \"Room: B?\"\n\"Dr. Q, Jr.\", \"Kid's Choice, Alpha\", \"Beta/2026\"\n\n",
    );
}

#[test]
fn let_bindings_can_build_a_complete_instance() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let s = +slot("s1");
let c1 = +choice("c1", bounds(1, 1));
let c2 = +choice("c2", bounds(1, 1));
let p1 = +chooser("p1", [1, 0]);
let p2 = +chooser("p2", [0, 1]);

+constraint(c1.slot == s);
+constraint(c2.slot == s);
+constraint(c1.choosers.contains(p1));
+constraint(c2.choosers.contains(p2));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"s1\"\n\"p1\", \"c1\"\n\"p2\", \"c2\"\n\n");
}

#[test]
fn if_expressions_can_conditionally_add_input() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let s1 = +slot("s1");
let s2 = +slot("s2");
let c1 = +choice("c1", bounds(1, 1));
let c2 = +choice("c2", bounds(1, 1));
+chooser("p", [0, 1]);

if true {
    +constraint(c1.slot == s2);
} else {
    +constraint(c1.slot == s1);
}

+constraint(c2.slot == s1);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s2\"\n\"c2\", \"s1\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p\", \"c2\", \"c1\"\n\n",
    );
}

#[test]
fn for_loops_can_generate_repeated_input() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let slot_names = ["s1", "s2", "s3"];
for name in slot_names {
    +slot(name);
}

let choice_names = ["c1", "c2", "c3"];
for name in choice_names {
    +choice(name, bounds(1, 1));
}

for index in [0, 1, 2] {
    +constraint(choice(choice_names[index]).slot == slot(slot_names[index]));
}

+chooser("p", [0, 1, 2]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\"c3\", \"s3\"\n\n\"Chooser\", \"s1\", \"s2\", \"s3\"\n\"p\", \"c1\", \"c2\", \"c3\"\n\n",
    );
}

#[test]
fn range_can_generate_preference_vectors() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+choice("c2", bounds(1, 1));
+choice("c1", bounds(1, 1));
+chooser("p1", range(0, 1));
+chooser("p2", [1, 0]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"s1\"\n\"p1\", \"c1\"\n\"p2\", \"c2\"\n\n");
}

#[test]
fn slice_and_end_can_select_subranges() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
let slot_names = ["ignore", "s1", "s2"];
for name in slot_names.slice(1, end) {
    +slot(name);
}

let choice_names = ["ignore", "c1", "c2"];
for name in choice_names.slice(1, end) {
    +choice(name, bounds(1, 1));
}

+chooser("p", [0, 1]);
+constraint(choice("c1").slot == slot("s1"));
+constraint(choice("c2").slot == slot("s2"));
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact(
        "\"Choice\", \"Slot\"\n\"c1\", \"s1\"\n\"c2\", \"s2\"\n\n\"Chooser\", \"s1\", \"s2\"\n\"p\", \"c1\", \"c2\"\n\n",
    );
}

#[test]
fn string_numeric_preferences_are_accepted() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+slot("s1");
+choice("c1", bounds(1, 1));
+choice("c2", bounds(1, 1));
+chooser("p1", ["1", "0"]);
+chooser("p2", ["0", "1"]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"s1\"\n\"p1\", \"c1\"\n\"p2\", \"c2\"\n\n");
}

#[test]
fn auto_generated_single_slot_is_used_when_no_slots_are_declared() {
    let output = run_cli(
        &["--timeout", "1s"],
        Some(
            r#"
+choice("c1", bounds(1, 1));
+chooser("p", [1]);
"#,
        ),
    );

    output.assert_success();
    output.assert_stdout_exact("\"Chooser\", \"Generated Slot\"\n\"p\", \"c1\"\n\n");
}
