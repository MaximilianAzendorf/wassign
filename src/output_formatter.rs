use crate::Solution;

/// Formats solutions as CSV output.
pub struct OutputFormatter;

/// A formatted CSV document returned by the formatter.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FormattedOutput(String);

impl FormattedOutput {
    /// Returns the formatted content as a string slice.
    #[must_use]
    pub fn as_str(&self) -> &str {
        &self.0
    }

    /// Converts the formatted content into an owned string.
    #[must_use]
    pub fn into_string(self) -> String {
        self.0
    }
}

impl std::ops::Deref for FormattedOutput {
    type Target = str;

    fn deref(&self) -> &Self::Target {
        self.as_str()
    }
}

impl std::fmt::Display for FormattedOutput {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(self.as_str())
    }
}

impl OutputFormatter {
    /// Formats the scheduling part of a solution as CSV.
    ///
    /// # Errors
    ///
    /// Returns an error if the solution does not contain a scheduling or if it
    /// references an invalid slot index.
    pub fn write_scheduling_solution(solution: &Solution) -> crate::Result<FormattedOutput> {
        let scheduling = solution
            .scheduling()
            .ok_or(crate::InputError::IncompleteSolution(
                "scheduling output requires a scheduling",
            ))?;
        let input = &scheduling.input_data;

        let mut lines = Vec::new();
        lines.push("\"Choice\", \"Slot\"".to_owned());
        for (choice_index, choice) in input.choices.iter().enumerate() {
            let slot = scheduling.slot_of(choice_index);
            let mut choice_name = choice.name.clone();
            let slot_name = if slot == crate::Scheduling::NOT_SCHEDULED {
                "not scheduled".to_owned()
            } else {
                input.slots[usize::try_from(slot).map_err(|_| {
                    crate::InputError::IncompleteSolution(
                        "scheduled choice referenced an invalid slot index",
                    )
                })?]
                .name
                .clone()
            };
            if let Some(stripped) = choice_name.strip_prefix(crate::InputData::GENERATED_PREFIX) {
                choice_name = stripped.to_owned();
            }
            lines.push(format!("\"{choice_name}\", \"{slot_name}\""));
        }

        Ok(FormattedOutput(lines.join("\n")))
    }

    /// Formats the assignment part of a solution as CSV.
    ///
    /// # Errors
    ///
    /// Returns an error if the solution does not contain both a scheduling and
    /// an assignment or if it references an invalid slot index.
    pub fn write_assignment_solution(solution: &Solution) -> crate::Result<FormattedOutput> {
        let scheduling = solution
            .scheduling()
            .ok_or(crate::InputError::IncompleteSolution(
                "assignment output requires a scheduling",
            ))?;
        let assignment = solution
            .assignment()
            .ok_or(crate::InputError::IncompleteSolution(
                "assignment output requires an assignment",
            ))?;
        let input = &assignment.input_data;

        let mut lines = Vec::new();
        let mut header = vec!["\"Chooser\"".to_owned()];
        header.extend(input.slots.iter().map(|slot| format!("\"{}\"", slot.name)));
        lines.push(header.join(", "));
        for chooser in 0..input.choosers.len() {
            let mut choices = vec![0_usize; input.slots.len()];
            for slot in 0..input.slots.len() {
                let choice = assignment.choice_of(chooser, slot);
                let scheduled_slot = usize::try_from(scheduling.slot_of(choice)).map_err(|_| {
                    crate::InputError::IncompleteSolution(
                        "assignment output referenced an unscheduled choice",
                    )
                })?;
                choices[scheduled_slot] = choice;
            }

            let mut row = vec![format!("\"{}\"", input.choosers[chooser].name)];
            for &choice in choices.iter().take(input.slots.len()) {
                let mut choice_name = input.choices[choice].name.clone();
                if let Some(stripped) = choice_name.strip_prefix(crate::InputData::GENERATED_PREFIX)
                {
                    choice_name = stripped.to_owned();
                }
                row.push(format!("\"{choice_name}\""));
            }
            lines.push(row.join(", "));
        }

        Ok(FormattedOutput(lines.join("\n")))
    }
}
