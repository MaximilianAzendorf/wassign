use std::sync::{Arc, Mutex};

use rhai::Engine;

use crate::{InputData, Options, Status};

use super::{
    constraint_expression::ConstraintExpression, input_builder::InputDataBuilder, input_chooser_data::InputChooserData,
    input_choice_data::InputChoiceData, input_slot_data::InputSlotData, rhai_interface::{ReaderState, RhaiInterface},
};

/// Parses the input DSL into processed [`crate::InputData`].
#[derive(Debug)]
pub struct InputReader {
    engine: Engine,
    options: Arc<Mutex<Options>>,
    pub(crate) set_map: std::collections::BTreeMap<String, Arc<InputSlotData>>,
    pub(crate) choice_map: std::collections::BTreeMap<String, Arc<InputChoiceData>>,
    pub(crate) chooser_map: std::collections::BTreeMap<String, Arc<InputChooserData>>,
    pub(crate) sets: Vec<Arc<InputSlotData>>,
    pub(crate) choices: Vec<Arc<InputChoiceData>>,
    pub(crate) choosers: Vec<Arc<InputChooserData>>,
    pub(crate) input_objects: Vec<super::input_object::InputObject>,
    pub(crate) constraint_expressions: Vec<ConstraintExpression>,
    state: Arc<Mutex<ReaderState>>,
}

impl InputReader {
    /// Creates a new input reader configured with the given options.
    #[must_use]
    pub fn new(options: &Arc<Options>) -> Self {
        let state = Arc::new(Mutex::new(ReaderState::default()));
        let options = Arc::new(Mutex::new(options.as_ref().clone()));
        let mut engine = Engine::new();
        RhaiInterface::register_interface(&mut engine, state.clone(), options.clone());
        Status::trace("Initialized Rhai input engine.");

        Self {
            engine,
            options,
            set_map: std::collections::BTreeMap::new(),
            choice_map: std::collections::BTreeMap::new(),
            chooser_map: std::collections::BTreeMap::new(),
            sets: Vec::new(),
            choices: Vec::new(),
            choosers: Vec::new(),
            input_objects: Vec::new(),
            constraint_expressions: Vec::new(),
            state,
        }
    }

    /// Parses a complete input document and returns the processed input data.
    ///
    /// # Errors
    ///
    /// Returns an error if the input cannot be parsed or validated.
    ///
    /// # Panics
    ///
    /// Panics if the internal reader state mutex is poisoned.
    pub fn read_input(&mut self, input: &str) -> crate::Result<Arc<InputData>> {
        Status::debug("Resetting input reader state.");
        *self.state.lock().expect("input reader state mutex poisoned") = ReaderState::default();
        let mut scope = RhaiInterface::build_scope();
        Status::debug(&format!("Evaluating input with {} line(s).", input.lines().count()));
        let _ = self.engine
            .eval_with_scope::<rhai::Dynamic>(&mut scope, input)
            .map_err(|err| crate::InputError::Message(err.to_string()))?;

        self.sync_from_state();
        Status::debug(&format!(
            "Reader registered {} slot(s), {} choice(s), {} chooser(s), and {} constraint expression(s).",
            self.sets.len(),
            self.choices.len(),
            self.choosers.len(),
            self.constraint_expressions.len()
        ));

        for object in &self.input_objects {
            if !object.registered {
                return Err(crate::InputError::Message(
                    "Newly created object not used. Did you try to access an existing one instead of creating a new one?"
                        .to_owned(),
                ));
            }
        }

        if self.choices.is_empty() {
            return Err(crate::InputError::Message("No choices defined in input.".to_owned()));
        }
        if self.choosers.is_empty() {
            return Err(crate::InputError::Message("No choosers defined in input.".to_owned()));
        }

        let mut builder = InputDataBuilder::default();
        builder.process_input_reader(self)?;
        Status::debug("Finished building input data.");
        Ok(builder.get_input_data())
    }

    fn sync_from_state(&mut self) {
        let state = self.state.lock().expect("input reader state mutex poisoned").clone();

        self.set_map = state
            .registered_set_ids
            .iter()
            .map(|&id| (state.slots[id].slot.name.clone(), Arc::new(state.slots[id].clone())))
            .collect();
        self.choice_map = state
            .registered_choice_ids
            .iter()
            .map(|&id| (state.choices[id].choice.name.clone(), Arc::new(state.choices[id].clone())))
            .collect();
        self.chooser_map = state
            .registered_chooser_ids
            .iter()
            .map(|&id| (state.choosers[id].chooser.name.clone(), Arc::new(state.choosers[id].clone())))
            .collect();
        self.sets = state
            .registered_set_ids
            .iter()
            .map(|&id| Arc::new(state.slots[id].clone()))
            .collect();
        self.choices = state
            .registered_choice_ids
            .iter()
            .map(|&id| Arc::new(state.choices[id].clone()))
            .collect();
        self.choosers = state
            .registered_chooser_ids
            .iter()
            .map(|&id| Arc::new(state.choosers[id].clone()))
            .collect();
        self.input_objects = state
            .slots
            .iter()
            .map(|slot| slot.object.clone())
            .chain(state.choices.iter().map(|choice| choice.object.clone()))
            .chain(state.choosers.iter().map(|chooser| chooser.object.clone()))
            .collect();
        self.constraint_expressions = state.constraint_expressions;
    }

    /// Returns the effective options after any `set_arguments(...)` calls in the input.
    ///
    /// # Panics
    ///
    /// Panics if the internal options mutex is poisoned.
    #[must_use]
    pub fn effective_options(&self) -> Arc<Options> {
        Arc::new(self.options.lock().expect("reader options mutex poisoned").clone())
    }
}

#[cfg(test)]
mod tests {
    use std::{
        fs,
        sync::Arc,
        time::{SystemTime, UNIX_EPOCH},
    };

    use crate::Options;

    use super::InputReader;

    fn temp_file_path(prefix: &str, suffix: &str) -> std::path::PathBuf {
        let stamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("system time should be after UNIX_EPOCH")
            .as_nanos();
        std::env::temp_dir().join(format!("wassign-{prefix}-{stamp}{suffix}"))
    }

    #[test]
    fn read_input_should_accept_let_and_preserve_string_literals() {
        let mut reader = InputReader::new(&Arc::new(Options::default()));
        let input = r#"
let s1 = +slot("var slot");
let w1 = +choice("var choice");
let p1 = +chooser("var chooser", [0]);

+constraint(w1.slot == s1);
+constraint(p1.choices.contains(w1));
"#;

        let data = reader.read_input(input).expect("input should parse");

        assert_eq!(data.slot_count(), 1);
        assert_eq!(data.choice_count(), 1);
        assert_eq!(data.chooser_count(), 1);
        assert_eq!(
            reader.set_map.keys().next().expect("slot should be registered"),
            "var slot"
        );
        assert_eq!(
            reader.choice_map.keys().next().expect("choice should be registered"),
            "var choice"
        );
        assert_eq!(
            reader.chooser_map.keys().next().expect("chooser should be registered"),
            "var chooser"
        );
    }

    #[test]
    fn read_input_should_apply_set_arguments() {
        let mut reader = InputReader::new(&Arc::new(Options::default()));
        let input = r#"
set_arguments(["-t", "5s", "-j", "4", "-n", "9", "--greedy"]);

+slot("s");
+choice("e");
+chooser("p", [0]);
"#;

        reader.read_input(input).expect("input should parse");

        let options = reader.options.lock().expect("reader options mutex poisoned");
        assert_eq!(options.timeout_seconds, 5);
        assert_eq!(options.thread_count, 4);
        assert_eq!(options.max_neighbors, 9);
        assert!(options.greedy);
    }

    #[test]
    fn read_input_should_accept_read_file() {
        let path = temp_file_path("read-file", ".txt");
        fs::write(&path, "from file").expect("temp file should be writable");

        let mut reader = InputReader::new(&Arc::new(Options::default()));
        let input = format!(
            r#"
let label = readFile("{}");

+slot(label);
+choice("e");
+chooser("p", [0]);
"#,
            path.display()
        );

        let data = reader.read_input(&input).expect("input should parse");
        assert_eq!(data.slot_count(), 1);
        assert_eq!(reader.set_map.keys().next().expect("slot should be registered"), "from file");

        let _ = fs::remove_file(path);
    }

    #[test]
    fn read_input_should_support_csv_row_accessors() {
        let path = temp_file_path("read-csv", ".csv");
        fs::write(
            &path,
            "\"Choice\",\"minimum\",\"maximum\",\"optional?\"\n\"A\",1,2,\"no\"\n\"B\",2,3,\"yes\"\n",
        )
        .expect("temp file should be writable");

        let mut reader = InputReader::new(&Arc::new(Options::default()));
        let input = format!(
            r#"
let file = read_csv("{}");
let first = file.row(1);
let second = file[2];

+choice(first[0], bounds(first[1], first[2]), optional_if(first[3] == "yes"));
+choice(second[0], bounds(second[1], second[2]), optional_if(second[3] == "yes"));
+chooser("p", [0, 0]);
"#,
            path.display()
        );

        let data = reader.read_input(&input).expect("input should parse");
        assert_eq!(data.choice_count(), 2);

        let _ = fs::remove_file(path);
    }

    #[test]
    fn read_input_should_support_part_aware_accessors() {
        let mut reader = InputReader::new(&Arc::new(Options::default()));
        let input = r#"
+slot("s1");
+slot("s2");
+choice("x", parts(2));
+chooser("p", [0]);

+constraint(choice("x").slot(1) == slot("s2"));
+constraint(choice("x").choosers(1).contains(chooser("p")));
"#;

        reader.read_input(input).expect("input should parse");

        assert_eq!(reader.constraint_expressions.len(), 2);
        assert_eq!(reader.constraint_expressions[0].left.part, 1);
        assert_eq!(reader.constraint_expressions[1].left.part, 1);
    }
}
