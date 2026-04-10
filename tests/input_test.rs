//! Integration tests for input parsing.

mod common;

use std::collections::BTreeSet;

use common::*;
use wassign::{ChoiceData, ChooserData, SlotData};

#[test]
fn should_parse_everything_without_error() {
    let input = r#"
+slot(".");
+slot("x");
+slot("a-b'c d");

+choice("w1", bounds(1, 9), optional);
+choice("w2", min(1), max(178), optional_if(false));
+choice("w3", bounds(12, 13), parts(3));

+chooser("p1", [0, 1, 2]);
+chooser("p2", [12, 100, 1]);
+chooser("p3", [10100, 12, 0]);
+chooser("p4", [20, 5, 21]);
+chooser("q5", [2, 4, 5]);

set_name(chooser("q5"), "p5");
"#;

    let data = parse_data(input);

    assert_eq!(data.slots.len(), 3);
    assert_eq!(data.choices.len(), 5);
    assert_eq!(data.choosers.len(), 5);
    assert_eq!(
        data.slots
            .iter()
            .map(SlotData::name)
            .collect::<BTreeSet<_>>(),
        BTreeSet::from([".", "a-b'c d", "x"])
    );
    assert_eq!(
        data.choices
            .iter()
            .map(ChoiceData::name)
            .filter(|name| !name.starts_with('~'))
            .collect::<BTreeSet<_>>(),
        BTreeSet::from(["w1", "w2", "w3"])
    );
    assert_eq!(
        data.choosers
            .iter()
            .map(ChooserData::name)
            .collect::<BTreeSet<_>>(),
        BTreeSet::from(["p1", "p2", "p3", "p4", "p5"])
    );
    assert_eq!(data.scheduling_constraints.len(), 6);
    assert_eq!(data.assignment_constraints.len(), 3);
}

#[test]
fn should_create_scheduling_constraints_for_multi_part_choices() {
    let input = r#"
+slot("s1");
+slot("s2");
+slot("s3");

+choice("e", bounds(1, 100), parts(3));

+chooser("p", [1]);
"#;

    let data = parse_data(input);
    assert_eq!(data.scheduling_constraints.len(), 6);
}

#[test]
fn should_auto_generate_set_if_none_is_given() {
    let input = r#"
+choice("w1", bounds(1, 100));
+chooser("p1", [0]);
"#;

    let data = parse_data(input);
    assert_eq!(data.slots.len(), 1);
}

#[test]
fn should_not_accept_too_many_preferences() {
    let input = r#"
+choice("w1", max(1));
+chooser("p1", [0, 1]);
"#;

    assert!(parse_data_result(input).is_err());
}

#[test]
fn should_not_accept_too_few_preferences() {
    let input = r#"
+choice("w1", max(1));
+choice("w2", max(1));
+chooser("p1", [0]);
"#;

    assert!(parse_data_result(input).is_err());
}

#[test]
fn should_not_accept_choices_with_zero_minimum_chooser_count() {
    let input = r#"
+slot("s")
+choice("e", max(100));
+chooser("p", [1, 1]);
"#;

    assert!(parse_data_result(input).is_err());
}

#[test]
fn should_not_accept_input_without_choices() {
    let input = r#"
+slot("s1");
+chooser("p1", []);
"#;

    assert!(parse_data_result(input).is_err());
}

#[test]
fn should_not_accept_input_without_choosers() {
    let input = r#"
+slot("s1");
+choice("w1", max(1));
"#;

    assert!(parse_data_result(input).is_err());
}

#[test]
fn should_not_accept_ambiguous_fuzzy_name_lookup() {
    let input = r#"
+slot("alpha beta");
+slot("alpha gamma");
+choice("w1", bounds(1, 1));
+chooser("p1", [0]);
+constraint(choice("w1").slot == slot("alpha"));
"#;

    let error = parse_data_result(input).expect_err("input should be rejected");
    assert!(error.to_string().contains("ambiguous"));
}

#[test]
fn should_not_accept_duplicate_names_after_renaming() {
    let input = r#"
let s1 = +slot("s1");
let s2 = slot("s2");
+set_name(s2, "s1");

+choice("w1", bounds(1, 1));
+chooser("p1", [0]);
"#;

    let error = parse_data_result(input).expect_err("input should be rejected");
    assert!(error.to_string().contains("Duplicate set name 's1'"));
}

#[test]
fn should_parse_slots() {
    let input = r#"
+slot("a");
+slot("b");

+choice("x", bounds(1, 1));
+chooser("y", [0]);
"#;

    let data = parse_data(input);
    assert_eq!(data.slots.len(), 2);
    assert_eq!(data.slots[0].name(), "a");
    assert_eq!(data.slots[1].name(), "b");
}

#[test]
fn should_parse_choices() {
    let input = r#"
+choice("a");
+choice("b", bounds(2, 3), optional);
+choice("c", bounds(5, 7), parts(2));

+chooser("y", [11, 13, 17]);
"#;

    let data = parse_data(input);
    assert_eq!(data.choices.len(), 4);
    assert_eq!(data.choices[0].name(), "a");
    assert_eq!(data.choices[0].min(), 1);
    assert_eq!(data.choices[0].max(), 1);
    assert_eq!(data.choices[0].continuation(), None);

    assert_eq!(data.choices[1].name(), "b");
    assert_eq!(data.choices[1].min(), 2);
    assert_eq!(data.choices[1].max(), 3);
    assert_eq!(data.choices[1].continuation(), None);
    assert!(data.choices[1].is_optional());

    assert_eq!(data.choices[2].name(), "c");
    assert_eq!(data.choices[2].min(), 5);
    assert_eq!(data.choices[2].max(), 7);
    assert_eq!(data.choices[2].continuation(), Some(3));

    assert!(data.choices[3].name().starts_with("~[2] c"));
    assert_eq!(data.choices[3].min(), 5);
    assert_eq!(data.choices[3].max(), 7);
    assert_eq!(data.choices[3].continuation(), None);

    assert_eq!(data.choosers[0].preferences(), &[6, 4, 0, 0]);
}

#[test]
fn should_parse_choosers() {
    let input = r#"
+choice("x");
+choice("y");

+chooser("a", [2, 3]);
+chooser("b", [5, 7]);
"#;

    let data = parse_data(input);
    assert_eq!(data.choosers.len(), 2);
    assert_eq!(data.choosers[0].name(), "a");
    assert_eq!(data.choosers[0].preferences(), &[5, 4]);
    assert_eq!(data.choosers[1].name(), "b");
    assert_eq!(data.choosers[1].preferences(), &[2, 0]);
}

#[test]
fn should_parse_constraints() {
    let input = r#"
let s1 = +slot("s1");
let s2 = +slot("s2");

let w1 = +choice("a");
let w2 = +choice("b");

let p1 = +chooser("c", [0, 0]);
let p2 = +chooser("d", [0, 0]);

+constraint(w2.slot == s2);
+constraint(w2.slot != s2);
+constraint(w2.slot == w1.slot);
+constraint(w1.slot != w2.slot);
+constraint(s2.size == 1);
+constraint(s2.size <= 3);

+constraint(s2.choices.contains(w1));
+constraint(s1.choices.contains_not(w2));
+constraint(s2.choices == s2.choices);

+constraint(w2.choosers == w1.choosers);
+constraint(p2.choices.contains(w2));
+constraint(p2.choices.contains_not(w2));
+constraint(p2.choices == p1.choices);

+constraint(w2.choosers.contains(p1));
+constraint(w1.choosers.contains_not(p2));
"#;

    let data = parse_data(input);
    let constraints = data
        .scheduling_constraints
        .iter()
        .chain(data.assignment_constraints.iter())
        .map(|constraint| format!("{constraint:?}"))
        .collect::<BTreeSet<_>>();
    let required = BTreeSet::from([
        "Constraint { kind: ChoiceIsInSlot, left: 0, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: ChoiceIsInSlot, left: 1, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: ChoiceIsNotInSlot, left: 1, right: 0, extra: 0 }".to_owned(),
        "Constraint { kind: ChoiceIsNotInSlot, left: 1, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: ChoicesAreInSameSlot, left: 1, right: 0, extra: 0 }".to_owned(),
        "Constraint { kind: ChoicesAreNotInSameSlot, left: 0, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: SlotHasLimitedSize, left: 1, right: 1, extra: 1 }".to_owned(),
        "Constraint { kind: SlotHasLimitedSize, left: 1, right: 3, extra: -2 }".to_owned(),
        "Constraint { kind: ChoicesHaveSameChoosers, left: 1, right: 0, extra: 0 }".to_owned(),
        "Constraint { kind: ChooserIsInChoice, left: 0, right: 0, extra: 0 }".to_owned(),
        "Constraint { kind: ChooserIsInChoice, left: 0, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: ChooserIsInChoice, left: 1, right: 0, extra: 0 }".to_owned(),
        "Constraint { kind: ChooserIsInChoice, left: 1, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: ChooserIsNotInChoice, left: 1, right: 0, extra: 0 }".to_owned(),
        "Constraint { kind: ChooserIsNotInChoice, left: 1, right: 1, extra: 0 }".to_owned(),
        "Constraint { kind: ChoosersHaveSameChoices, left: 1, right: 0, extra: 0 }".to_owned(),
    ]);
    assert_eq!(constraints, required);
}

#[test]
fn should_accept_string_preferences_and_range() {
    let input = r#"
+choice("a");
+choice("b");

+chooser("p", ["1", "0"]);
+chooser("q", range(0, 1));
"#;

    let data = parse_data(input);
    assert_eq!(data.choices.len(), 2);
    assert_eq!(data.choosers.len(), 2);
}

#[test]
fn should_parse_csv_input_with_slice_and_numeric_strings() {
    let csv_path = write_temp_file(
        "input-test",
        ".csv",
        "\"Choice\",\"minimum\",\"maximum\",\"optional?\"\n\"A\",1,2,\"no\"\n\"B\",2,3,\"yes\"\n",
    );
    let input = format!(
        r#"
let file = read_csv("{}");

for row in file.rows.slice(1, end) {{
    +choice(row[0], bounds(row[1], row[2]), optional_if(row[3] == "yes"));
}}

+chooser("p", [0, 0]);
"#,
        csv_path.display()
    );

    let data = parse_data(&input);
    assert_eq!(data.choices.len(), 2);
    assert_eq!(data.choosers.len(), 1);

    let _ = std::fs::remove_file(csv_path);
}
