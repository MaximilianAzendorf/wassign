use std::sync::{Arc, Mutex};

use rhai::{Array, Dynamic, Engine, EvalAltResult, Scope};

use crate::{ChooserData, InputError, Options, SlotData};

use super::constraint_expression::{
    AccessorType, ConstraintExpression, ConstraintExpressionAccessor, ConstraintExpressionRelation,
    RelationType,
};
use super::fuzzy_match::FuzzyMatch;
use super::input_choice_data::InputChoiceData;
use super::input_chooser_data::InputChooserData;
use super::input_object::InputObject;
use super::input_slot_data::InputSlotData;
use super::proto_choice_data::ProtoChoiceData;
use super::tagged::{Tag, Tagged};

#[derive(Debug, Clone, Default)]
pub struct ReaderState {
    pub slots: Vec<InputSlotData>,
    pub choices: Vec<InputChoiceData>,
    pub choosers: Vec<InputChooserData>,
    pub registered_set_ids: Vec<usize>,
    pub registered_choice_ids: Vec<usize>,
    pub registered_chooser_ids: Vec<usize>,
    pub constraint_expressions: Vec<ConstraintExpression>,
}

#[derive(Debug, Clone)]
struct CsvDocument {
    rows: Vec<Vec<String>>,
}

#[derive(Debug, Clone)]
struct SlotRef {
    state: Arc<Mutex<ReaderState>>,
    id: usize,
}

#[derive(Debug, Clone)]
struct ChoiceRef {
    state: Arc<Mutex<ReaderState>>,
    id: usize,
}

#[derive(Debug, Clone)]
struct ChooserRef {
    state: Arc<Mutex<ReaderState>>,
    id: usize,
}

pub struct RhaiInterface;

type ScriptResult<T> = std::result::Result<T, Box<EvalAltResult>>;

impl RhaiInterface {
    pub fn register_interface(
        engine: &mut Engine,
        state: &Arc<Mutex<ReaderState>>,
        options: &Arc<Mutex<Options>>,
    ) {
        engine.register_type_with_name::<Tagged>("Tagged");
        engine.register_type_with_name::<SlotRef>("slot");
        engine.register_type_with_name::<ChoiceRef>("choice");
        engine.register_type_with_name::<ChooserRef>("chooser");
        engine.register_type_with_name::<CsvDocument>("csvDoc");
        engine.register_type_with_name::<ConstraintExpressionAccessor>("__cexpAccessor");
        engine.register_type_with_name::<ConstraintExpression>("__cexp");

        engine.register_fn("slot", {
            let state = state.clone();
            move |name: &str| script_result(slot(state.clone(), name))
        });
        engine.register_fn("choice", {
            let state = state.clone();
            move |name: &str| script_result(choice(state.clone(), name, vec![]))
        });
        engine.register_fn("choice", {
            let state = state.clone();
            move |name: &str, tag1: Tagged| script_result(choice(state.clone(), name, vec![tag1]))
        });
        engine.register_fn("choice", {
            let state = state.clone();
            move |name: &str, tag1: Tagged, tag2: Tagged| {
                script_result(choice(state.clone(), name, vec![tag1, tag2]))
            }
        });
        engine.register_fn("choice", {
            let state = state.clone();
            move |name: &str, tag1: Tagged, tag2: Tagged, tag3: Tagged| {
                script_result(choice(state.clone(), name, vec![tag1, tag2, tag3]))
            }
        });
        engine.register_fn("choice", {
            let state = state.clone();
            move |name: &str, tag1: Tagged, tag2: Tagged, tag3: Tagged, tag4: Tagged| {
                script_result(choice(state.clone(), name, vec![tag1, tag2, tag3, tag4]))
            }
        });
        engine.register_fn("chooser", {
            let state = state.clone();
            move |name: &str| script_result(chooser(state.clone(), name, vec![]))
        });
        engine.register_fn("chooser", {
            let state = state.clone();
            move |name: &str, prefs: Array| {
                script_result(
                    array_to_i32(&prefs).and_then(|prefs| chooser(state.clone(), name, prefs)),
                )
            }
        });
        engine.register_fn("set_arguments", {
            let options = options.clone();
            move |args: Array| script_result(set_arguments(&options, args))
        });
        engine.register_fn("set_name", |mut slot_ref: SlotRef, name: &str| {
            slot_ref.set_name(name.to_owned());
            slot_ref
        });
        engine.register_fn("set_name", |mut choice_ref: ChoiceRef, name: &str| {
            choice_ref.set_name(name.to_owned());
            choice_ref
        });
        engine.register_fn("set_name", |mut chooser_ref: ChooserRef, name: &str| {
            chooser_ref.set_name(name.to_owned());
            chooser_ref
        });

        engine.register_fn("+", |slot_ref: SlotRef| {
            script_result(register_slot(slot_ref))
        });
        engine.register_fn("+", |choice_ref: ChoiceRef| {
            script_result(register_choice(choice_ref))
        });
        engine.register_fn("+", |chooser_ref: ChooserRef| {
            script_result(register_chooser(chooser_ref))
        });
        engine.register_fn("+", {
            let state = state.clone();
            move |expression: ConstraintExpression| {
                script_result(register_constraint(&state, expression))
            }
        });
        engine.register_fn("constraint", |expression: ConstraintExpression| expression);

        engine.register_get_set("name", SlotRef::name, SlotRef::set_name);
        engine.register_get("choices", SlotRef::choices_accessor);
        engine.register_get("size", SlotRef::size_accessor);

        engine.register_get_set("name", ChoiceRef::name, ChoiceRef::set_name);
        engine.register_get("slot", ChoiceRef::slot_accessor);
        engine.register_fn("slot", |choice_ref: ChoiceRef, part: i64| {
            script_result(choice_ref.slot_accessor_part(part))
        });
        engine.register_get("choosers", ChoiceRef::choosers_accessor);
        engine.register_fn("choosers", |choice_ref: ChoiceRef, part: i64| {
            script_result(choice_ref.choosers_accessor_part(part))
        });

        engine.register_get_set("name", ChooserRef::name, ChooserRef::set_name);
        engine.register_get("choices", ChooserRef::choices_accessor);

        engine.register_get("rows", CsvDocument::rows_accessor);
        engine.register_fn("row", |document: &mut CsvDocument, index: i64| {
            script_result(document.row_accessor(index))
        });
        engine.register_indexer_get(CsvDocument::index_accessor);

        register_relation(engine, "==", RelationType::Eq);
        register_relation(engine, "!=", RelationType::Neq);
        register_relation(engine, ">", RelationType::Gt);
        register_relation(engine, "<", RelationType::Lt);
        register_relation(engine, ">=", RelationType::Geq);
        register_relation(engine, "<=", RelationType::Leq);
        register_contains(engine, "contains", RelationType::Contains);
        register_contains(engine, "contains_not", RelationType::NotContains);

        engine.register_fn("+", |left: String, right: i64| format!("{left}{right}"));
        engine.register_fn("+", |left: i64, right: String| format!("{left}{right}"));

        engine.register_fn("min", |value: i64| {
            Tagged::new(Tag::Min, vec![value as i32])
        });
        engine.register_fn("min", |value: String| {
            script_result(parse_string_int(&value).map(|value| Tagged::new(Tag::Min, vec![value])))
        });
        engine.register_fn("max", |value: i64| {
            Tagged::new(Tag::Max, vec![value as i32])
        });
        engine.register_fn("max", |value: String| {
            script_result(parse_string_int(&value).map(|value| Tagged::new(Tag::Max, vec![value])))
        });
        engine.register_fn("parts", |value: i64| {
            Tagged::new(Tag::Parts, vec![value as i32])
        });
        engine.register_fn("parts", |value: String| {
            script_result(
                parse_string_int(&value).map(|value| Tagged::new(Tag::Parts, vec![value])),
            )
        });
        engine.register_fn("bounds", |min: i64, max: i64| {
            Tagged::new(Tag::Bounds, vec![min as i32, max as i32])
        });
        engine.register_fn("bounds", |min: String, max: String| {
            script_result(parse_string_int(&min).and_then(|min| {
                parse_string_int(&max).map(|max| Tagged::new(Tag::Bounds, vec![min, max]))
            }))
        });
        engine.register_fn("optional_if", |value: bool| {
            Tagged::new(Tag::Optional, vec![i32::from(value)])
        });

        engine.register_fn("readFile", |filename: String| {
            script_result(read_file_string(&filename))
        });
        engine.register_fn("read_csv", |filename: String| {
            script_result(read_file_csv(&filename, ','))
        });
        engine.register_fn("read_csv", |filename: String, separator: String| {
            let separator = separator.chars().next().unwrap_or(',');
            script_result(read_file_csv(&filename, separator))
        });
        engine.register_fn("read_csv", |filename: String, separator: char| {
            script_result(read_file_csv(&filename, separator))
        });
        engine.register_fn("range", |from: i64, to: i64| range(from, to));
        engine.register_fn("slice", |array: Array, from: i64, to: i64| {
            script_result(slice(&array, from, to))
        });
    }

    pub fn build_scope() -> Scope<'static> {
        let mut scope = Scope::new();
        scope.push_constant("optional", Tagged::new(Tag::Optional, vec![1]));
        scope.push_constant("end", i64::MAX);
        scope
    }
}

fn script_result<T>(result: crate::Result<T>) -> ScriptResult<T> {
    result.map_err(|err| err.to_string().into())
}

fn reader_state(
    state: &Arc<Mutex<ReaderState>>,
) -> crate::Result<std::sync::MutexGuard<'_, ReaderState>> {
    state
        .lock()
        .map_err(|_| InputError::Message("reader state mutex poisoned".to_owned()))
}

fn reader_options(
    options: &Arc<Mutex<Options>>,
) -> crate::Result<std::sync::MutexGuard<'_, Options>> {
    options
        .lock()
        .map_err(|_| InputError::Message("reader options mutex poisoned".to_owned()))
}

fn register_relation(engine: &mut Engine, op: &str, relation: RelationType) {
    engine.register_fn(
        op,
        move |left: ConstraintExpressionAccessor, right: ConstraintExpressionAccessor| {
            build_expression(left, relation, right)
        },
    );
    engine.register_fn(
        op,
        move |left: ConstraintExpressionAccessor, right: SlotRef| {
            build_expression(left, relation, slot_to_accessor(&right))
        },
    );
    engine.register_fn(
        op,
        move |left: SlotRef, right: ConstraintExpressionAccessor| {
            build_expression(slot_to_accessor(&left), relation, right)
        },
    );
    engine.register_fn(
        op,
        move |left: ConstraintExpressionAccessor, right: ChoiceRef| {
            build_expression(left, relation, choice_to_accessor(&right))
        },
    );
    engine.register_fn(
        op,
        move |left: ChoiceRef, right: ConstraintExpressionAccessor| {
            build_expression(choice_to_accessor(&left), relation, right)
        },
    );
    engine.register_fn(
        op,
        move |left: ConstraintExpressionAccessor, right: ChooserRef| {
            build_expression(left, relation, chooser_to_accessor(&right))
        },
    );
    engine.register_fn(
        op,
        move |left: ChooserRef, right: ConstraintExpressionAccessor| {
            build_expression(chooser_to_accessor(&left), relation, right)
        },
    );
    engine.register_fn(op, move |left: ConstraintExpressionAccessor, right: i64| {
        build_expression(left, relation, integer_accessor(right))
    });
    engine.register_fn(op, move |left: i64, right: ConstraintExpressionAccessor| {
        build_expression(integer_accessor(left), relation, right)
    });
}

fn register_contains(engine: &mut Engine, name: &str, relation: RelationType) {
    engine.register_fn(
        name,
        move |left: ConstraintExpressionAccessor, right: SlotRef| {
            build_expression(left, relation, slot_to_accessor(&right))
        },
    );
    engine.register_fn(
        name,
        move |left: ConstraintExpressionAccessor, right: ChoiceRef| {
            build_expression(left, relation, choice_to_accessor(&right))
        },
    );
    engine.register_fn(
        name,
        move |left: ConstraintExpressionAccessor, right: ChooserRef| {
            build_expression(left, relation, chooser_to_accessor(&right))
        },
    );
}

fn build_expression(
    left: ConstraintExpressionAccessor,
    relation: RelationType,
    right: ConstraintExpressionAccessor,
) -> ConstraintExpression {
    ConstraintExpression {
        left,
        relation: ConstraintExpressionRelation { kind: relation },
        right,
    }
}

fn slot(state: Arc<Mutex<ReaderState>>, name: &str) -> crate::Result<SlotRef> {
    if let Some(id) = find_registered_slot(&state, name)? {
        return Ok(SlotRef { state, id });
    }

    let mut state_guard = reader_state(&state)?;
    let id = state_guard.slots.len();
    state_guard.slots.push(InputSlotData {
        slot: SlotData {
            name: name.to_owned(),
        },
        object: InputObject { registered: false },
    });
    drop(state_guard);
    Ok(SlotRef { state, id })
}

fn choice(
    state: Arc<Mutex<ReaderState>>,
    name: &str,
    tags: Vec<Tagged>,
) -> crate::Result<ChoiceRef> {
    if let Some(id) = find_registered_choice(&state, name)? {
        return Ok(ChoiceRef { state, id });
    }

    let mut proto = ProtoChoiceData {
        name: name.to_owned(),
        min: 1,
        max: 1,
        parts: 1,
        optional: false,
    };
    for tag in tags {
        match tag.tag {
            Tag::Min => proto.min = tag.value(0),
            Tag::Max => proto.max = tag.value(0),
            Tag::Bounds => {
                proto.min = tag.value(0);
                proto.max = tag.value(1);
            }
            Tag::Parts => proto.parts = tag.value(0),
            Tag::Optional => proto.optional = tag.value(0) == 1,
        }
    }

    let mut state_guard = reader_state(&state)?;
    let id = state_guard.choices.len();
    state_guard.choices.push(InputChoiceData {
        choice: proto,
        object: InputObject { registered: false },
    });
    drop(state_guard);
    Ok(ChoiceRef { state, id })
}

fn chooser(
    state: Arc<Mutex<ReaderState>>,
    name: &str,
    preferences: Vec<i32>,
) -> crate::Result<ChooserRef> {
    if let Some(id) = find_registered_chooser(&state, name)? {
        return Ok(ChooserRef { state, id });
    }

    let mut state_guard = reader_state(&state)?;
    let id = state_guard.choosers.len();
    state_guard.choosers.push(InputChooserData {
        chooser: ChooserData {
            name: name.to_owned(),
            preferences,
        },
        object: InputObject { registered: false },
    });
    drop(state_guard);
    Ok(ChooserRef { state, id })
}

fn set_arguments(options: &Arc<Mutex<Options>>, args: Array) -> crate::Result<()> {
    let mut values = Vec::with_capacity(args.len() + 1);
    values.push("wassign".to_owned());
    for arg in args {
        let text = arg
            .clone()
            .try_cast::<String>()
            .ok_or_else(|| InputError::Message("set_arguments expects strings.".to_owned()))?;
        values.push(text);
    }

    let mut parsed = reader_options(options)?;
    let mut next = parsed.clone();

    let mut iter = values.iter().skip(1);
    while let Some(arg) = iter.next() {
        match arg.as_str() {
            "-i" | "--input" => {
                let value = iter
                    .next()
                    .ok_or_else(|| InputError::Message("Missing value for --input.".to_owned()))?;
                next.input_files.push(value.clone());
            }
            "-o" | "--output" => {
                let value = iter
                    .next()
                    .ok_or_else(|| InputError::Message("Missing value for --output.".to_owned()))?;
                next.output_file = Some(value.clone());
            }
            "-a" | "--any" => next.any = true,
            "-p" | "--pref-exp" => {
                let value = iter.next().ok_or_else(|| {
                    InputError::Message("Missing value for --pref-exp.".to_owned())
                })?;
                next.preference_exponent = value
                    .parse::<f64>()
                    .map_err(|err| InputError::Message(err.to_string()))?;
            }
            "-t" | "--timeout" => {
                let value = iter.next().ok_or_else(|| {
                    InputError::Message("Missing value for --timeout.".to_owned())
                })?;
                next.timeout_seconds = Options::parse_time(value)?;
            }
            "--cs-timeout" => {
                let value = iter.next().ok_or_else(|| {
                    InputError::Message("Missing value for --cs-timeout.".to_owned())
                })?;
                next.critical_set_timeout_seconds = Options::parse_time(value)?;
            }
            "--no-cs" => next.no_critical_sets = true,
            "--no-cs-simp" => next.no_critical_set_simplification = true,
            "-j" | "--threads" => {
                let value = iter.next().ok_or_else(|| {
                    InputError::Message("Missing value for --threads.".to_owned())
                })?;
                next.thread_count = value
                    .parse::<i32>()
                    .map_err(|err| InputError::Message(err.to_string()))?;
            }
            "-n" | "--max-neighbors" => {
                let value = iter.next().ok_or_else(|| {
                    InputError::Message("Missing value for --max-neighbors.".to_owned())
                })?;
                next.max_neighbors = value
                    .parse::<i32>()
                    .map_err(|err| InputError::Message(err.to_string()))?;
            }
            "-g" | "--greedy" => next.greedy = true,
            "-h" | "--help" | "--version" => {}
            other => return Err(InputError::Message(format!("Unknown argument '{other}'."))),
        }
    }

    *parsed = next;
    Ok(())
}

fn register_slot(slot_ref: SlotRef) -> crate::Result<SlotRef> {
    let mut state = reader_state(&slot_ref.state)?;
    let name = state.slots[slot_ref.id].slot.name.clone();
    if state
        .registered_set_ids
        .iter()
        .any(|&id| id != slot_ref.id && state.slots[id].slot.name == name)
    {
        return Err(InputError::Message(format!("Duplicate set name '{name}'.")));
    }
    if !state.registered_set_ids.contains(&slot_ref.id) {
        state.registered_set_ids.push(slot_ref.id);
        state.slots[slot_ref.id].object.registered = true;
    }
    drop(state);
    Ok(slot_ref)
}

fn register_choice(choice_ref: ChoiceRef) -> crate::Result<ChoiceRef> {
    let mut state = reader_state(&choice_ref.state)?;
    let name = state.choices[choice_ref.id].choice.name.clone();
    if state
        .registered_choice_ids
        .iter()
        .any(|&id| id != choice_ref.id && state.choices[id].choice.name == name)
    {
        return Err(InputError::Message(format!(
            "Duplicate choice name '{name}'."
        )));
    }
    if !state.registered_choice_ids.contains(&choice_ref.id) {
        state.registered_choice_ids.push(choice_ref.id);
        state.choices[choice_ref.id].object.registered = true;
    }
    drop(state);
    Ok(choice_ref)
}

fn register_chooser(chooser_ref: ChooserRef) -> crate::Result<ChooserRef> {
    let mut state = reader_state(&chooser_ref.state)?;
    let name = state.choosers[chooser_ref.id].chooser.name.clone();
    if state
        .registered_chooser_ids
        .iter()
        .any(|&id| id != chooser_ref.id && state.choosers[id].chooser.name == name)
    {
        return Err(InputError::Message(format!(
            "Duplicate chooser name '{name}'."
        )));
    }
    if !state.registered_chooser_ids.contains(&chooser_ref.id) {
        state.registered_chooser_ids.push(chooser_ref.id);
        state.choosers[chooser_ref.id].object.registered = true;
    }
    drop(state);
    Ok(chooser_ref)
}

fn register_constraint(
    state: &Arc<Mutex<ReaderState>>,
    expression: ConstraintExpression,
) -> crate::Result<ConstraintExpression> {
    reader_state(state)?
        .constraint_expressions
        .push(expression.clone());
    Ok(expression)
}

fn find_registered_slot(
    state: &Arc<Mutex<ReaderState>>,
    name: &str,
) -> crate::Result<Option<usize>> {
    find_registered_name(state, name, |state| {
        state
            .registered_set_ids
            .iter()
            .map(|&id| (id, state.slots[id].slot.name.clone()))
            .collect()
    })
}

fn find_registered_choice(
    state: &Arc<Mutex<ReaderState>>,
    name: &str,
) -> crate::Result<Option<usize>> {
    find_registered_name(state, name, |state| {
        state
            .registered_choice_ids
            .iter()
            .map(|&id| (id, state.choices[id].choice.name.clone()))
            .collect()
    })
}

fn find_registered_chooser(
    state: &Arc<Mutex<ReaderState>>,
    name: &str,
) -> crate::Result<Option<usize>> {
    find_registered_name(state, name, |state| {
        state
            .registered_chooser_ids
            .iter()
            .map(|&id| (id, state.choosers[id].chooser.name.clone()))
            .collect()
    })
}

fn find_registered_name(
    state: &Arc<Mutex<ReaderState>>,
    name: &str,
    values: impl Fn(&ReaderState) -> Vec<(usize, String)>,
) -> crate::Result<Option<usize>> {
    let state = reader_state(state)?;
    let values = values(&state);
    let names = values
        .iter()
        .map(|(_, value)| value.clone())
        .collect::<Vec<_>>();
    let matches = FuzzyMatch::find(name, &names);
    if matches.len() > 1 {
        return Err(InputError::Message(format!(
            "The name \"{name}\" is ambiguous."
        )));
    }
    Ok(matches.first().map(|&index| values[index].0))
}

fn array_to_i32(array: &Array) -> crate::Result<Vec<i32>> {
    array
        .iter()
        .map(|value| {
            if let Some(number) = value.clone().try_cast::<i64>() {
                i32::try_from(number).map_err(|err| InputError::Message(err.to_string()))
            } else if let Some(number) = value.clone().try_cast::<i32>() {
                Ok(number)
            } else if let Some(text) = value.clone().try_cast::<String>() {
                parse_string_int(&text)
            } else {
                Err(InputError::Message(
                    "Unsupported chooser preference value.".to_owned(),
                ))
            }
        })
        .collect()
}

fn parse_string_int(value: &str) -> crate::Result<i32> {
    let number = value
        .parse::<i64>()
        .map_err(|err| InputError::Message(err.to_string()))?;
    i32::try_from(number).map_err(|err| InputError::Message(err.to_string()))
}

fn read_file_string(filename: &str) -> crate::Result<String> {
    std::fs::read_to_string(filename).map_err(|err| InputError::Message(err.to_string()))
}

fn read_file_csv(filename: &str, separator: char) -> crate::Result<CsvDocument> {
    let delimiter = u8::try_from(u32::from(separator))
        .map_err(|_| InputError::Message("CSV separator must fit in one byte.".to_owned()))?;
    let mut reader = csv::ReaderBuilder::new()
        .has_headers(false)
        .delimiter(delimiter)
        .from_path(filename)
        .map_err(|err| InputError::Message(err.to_string()))?;

    let mut rows = Vec::new();
    for record in reader.records() {
        let record = record.map_err(|err| InputError::Message(err.to_string()))?;
        rows.push(record.iter().map(str::to_owned).collect());
    }

    Ok(CsvDocument { rows })
}

fn range(from: i64, to: i64) -> Array {
    let mut result = Array::new();
    if from <= to {
        for value in from..=to {
            result.push(Dynamic::from_int(value));
        }
    }
    result
}

fn slice(values: &Array, from: i64, to: i64) -> crate::Result<Array> {
    if from < 0 || to < 0 {
        return Err(InputError::Message(format!(
            "An array of length {} can not be sliced between {from} and {to}.",
            values.len()
        )));
    }

    let from = usize::try_from(from).map_err(|err| InputError::Message(err.to_string()))?;
    if from >= values.len() {
        return Err(InputError::Message(format!(
            "An array of length {} can not be sliced between {from} and {to}.",
            values.len()
        )));
    }

    let to = if to == i64::MAX {
        values.len().saturating_sub(1)
    } else {
        let to = usize::try_from(to).map_err(|err| InputError::Message(err.to_string()))?;
        if to >= values.len() {
            return Err(InputError::Message(format!(
                "An array of length {} can not be sliced between {from} and {to}.",
                values.len()
            )));
        }
        to
    };

    if from > to {
        return Ok(Array::new());
    }

    Ok(values[from..=to].to_vec())
}

impl CsvDocument {
    fn rows_accessor(&mut self) -> Array {
        self.rows.iter().cloned().map(row_to_dynamic).collect()
    }

    fn row_accessor(&self, index: i64) -> crate::Result<Array> {
        let index = usize::try_from(index).map_err(|err| InputError::Message(err.to_string()))?;
        let row = self
            .rows
            .get(index)
            .ok_or_else(|| InputError::Message(format!("CSV row {index} is out of bounds.")))?;
        Ok(row_to_array(row.clone()))
    }

    fn index_accessor(&mut self, index: i64) -> Array {
        let Ok(index) = usize::try_from(index) else {
            return Array::new();
        };
        self.rows
            .get(index)
            .cloned()
            .map_or_else(Array::new, row_to_array)
    }
}

fn row_to_dynamic(row: Vec<String>) -> Dynamic {
    Dynamic::from(row_to_array(row))
}

fn row_to_array(row: Vec<String>) -> Array {
    row.into_iter().map(Dynamic::from).collect()
}

fn slot_to_accessor(slot_ref: &SlotRef) -> ConstraintExpressionAccessor {
    ConstraintExpressionAccessor {
        kind: AccessorType::Slot,
        sub_type: AccessorType::NotSet,
        name: slot_ref.name_value(),
        part: 0,
    }
}

fn choice_to_accessor(choice_ref: &ChoiceRef) -> ConstraintExpressionAccessor {
    ConstraintExpressionAccessor {
        kind: AccessorType::Choice,
        sub_type: AccessorType::NotSet,
        name: choice_ref.name_value(),
        part: 0,
    }
}

fn chooser_to_accessor(chooser_ref: &ChooserRef) -> ConstraintExpressionAccessor {
    ConstraintExpressionAccessor {
        kind: AccessorType::Chooser,
        sub_type: AccessorType::NotSet,
        name: chooser_ref.name_value(),
        part: 0,
    }
}

fn integer_accessor(value: i64) -> ConstraintExpressionAccessor {
    ConstraintExpressionAccessor {
        kind: AccessorType::Integer,
        sub_type: AccessorType::NotSet,
        name: value.to_string(),
        part: 0,
    }
}

impl SlotRef {
    fn name_value(&self) -> String {
        self.state
            .lock()
            .expect("reader state mutex poisoned")
            .slots[self.id]
            .slot
            .name
            .clone()
    }

    fn name(&mut self) -> String {
        self.name_value()
    }

    fn set_name(&mut self, name: String) {
        self.state
            .lock()
            .expect("reader state mutex poisoned")
            .slots[self.id]
            .slot
            .name = name;
    }

    fn choices_accessor(&mut self) -> ConstraintExpressionAccessor {
        ConstraintExpressionAccessor {
            kind: AccessorType::Slot,
            sub_type: AccessorType::Choice,
            name: self.name(),
            part: 0,
        }
    }

    fn size_accessor(&mut self) -> ConstraintExpressionAccessor {
        ConstraintExpressionAccessor {
            kind: AccessorType::Slot,
            sub_type: AccessorType::Size,
            name: self.name(),
            part: 0,
        }
    }
}

impl ChoiceRef {
    fn name_value(&self) -> String {
        self.state
            .lock()
            .expect("reader state mutex poisoned")
            .choices[self.id]
            .choice
            .name
            .clone()
    }

    fn name(&mut self) -> String {
        self.name_value()
    }

    fn set_name(&mut self, name: String) {
        self.state
            .lock()
            .expect("reader state mutex poisoned")
            .choices[self.id]
            .choice
            .name = name;
    }

    fn slot_accessor(&mut self) -> ConstraintExpressionAccessor {
        ConstraintExpressionAccessor {
            kind: AccessorType::Choice,
            sub_type: AccessorType::Slot,
            name: self.name(),
            part: 0,
        }
    }

    fn choosers_accessor(&mut self) -> ConstraintExpressionAccessor {
        ConstraintExpressionAccessor {
            kind: AccessorType::Choice,
            sub_type: AccessorType::Chooser,
            name: self.name(),
            part: 0,
        }
    }

    fn slot_accessor_part(&self, part: i64) -> crate::Result<ConstraintExpressionAccessor> {
        Ok(ConstraintExpressionAccessor {
            kind: AccessorType::Choice,
            sub_type: AccessorType::Slot,
            name: self.name_value(),
            part: usize::try_from(part).map_err(|err| InputError::Message(err.to_string()))?,
        })
    }

    fn choosers_accessor_part(&self, part: i64) -> crate::Result<ConstraintExpressionAccessor> {
        Ok(ConstraintExpressionAccessor {
            kind: AccessorType::Choice,
            sub_type: AccessorType::Chooser,
            name: self.name_value(),
            part: usize::try_from(part).map_err(|err| InputError::Message(err.to_string()))?,
        })
    }
}

impl ChooserRef {
    fn name_value(&self) -> String {
        self.state
            .lock()
            .expect("reader state mutex poisoned")
            .choosers[self.id]
            .chooser
            .name
            .clone()
    }

    fn name(&mut self) -> String {
        self.name_value()
    }

    fn set_name(&mut self, name: String) {
        self.state
            .lock()
            .expect("reader state mutex poisoned")
            .choosers[self.id]
            .chooser
            .name = name;
    }

    fn choices_accessor(&mut self) -> ConstraintExpressionAccessor {
        ConstraintExpressionAccessor {
            kind: AccessorType::Chooser,
            sub_type: AccessorType::Choice,
            name: self.name(),
            part: 0,
        }
    }
}
