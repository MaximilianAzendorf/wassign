#![expect(
    clippy::struct_excessive_bools,
    clippy::too_many_lines,
    reason = "the CLI shape is intentionally flat and mirrors the executable flags"
)]

use clap::{Arg, ArgAction, Command, error::ErrorKind};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Outcome of command-line parsing.
pub enum OptionsParseStatus {
    /// Parsing succeeded and execution should continue.
    Ok,
    /// Parsing requested an early exit, for example after `--help`.
    Exit,
    /// Parsing failed.
    Error,
}

#[derive(Debug, Clone)]
/// Command-line options for the `wassign` executable.
pub struct Options {
    /// Input files to read. When empty, input is read from standard input.
    pub input_files: Vec<String>,
    /// Output path prefix used for generated CSV files.
    pub output_file: String,
    /// Whether any valid solution is acceptable.
    pub any: bool,
    /// Exponent used when converting preferences into assignment costs.
    pub preference_exponent: f64,
    /// Overall solver timeout in seconds.
    pub timeout_seconds: i32,
    /// Timeout for critical-set analysis in seconds.
    pub critical_set_timeout_seconds: i32,
    /// Disables critical-set analysis.
    pub no_critical_sets: bool,
    /// Disables simplification during critical-set analysis.
    pub no_critical_set_simplification: bool,
    /// Number of worker threads to use.
    pub thread_count: i32,
    /// Maximum number of hill-climbing neighbors to inspect per step.
    pub max_neighbors: i32,
    /// Enables greedy assignment solving.
    pub greedy: bool,
}

impl Default for Options {
    fn default() -> Self {
        Self {
            input_files: Vec::new(),
            output_file: String::new(),
            any: false,
            preference_exponent: 3.0,
            timeout_seconds: 60,
            critical_set_timeout_seconds: 3,
            no_critical_sets: false,
            no_critical_set_simplification: false,
            thread_count: std::thread::available_parallelism()
                .map_or(1_i32, |value| i32::try_from(value.get()).unwrap_or(1)),
            max_neighbors: 12,
            greedy: false,
        }
    }
}

impl Options {
    pub(crate) fn parse_time(value: &str) -> crate::Result<i32> {
        let mut total = 0_i32;
        let mut current = 0_i32;

        for ch in value.chars() {
            if ch.is_ascii_digit() {
                current = current
                    .checked_mul(10)
                    .and_then(|value| value.checked_add((ch as i32) - ('0' as i32)))
                    .ok_or_else(|| crate::InputError::Message(format!("Unknown time specifier {value}.")))?;
            } else if ch.is_ascii_lowercase() {
                let multiplier = match ch {
                    's' => 1,
                    'm' => 60,
                    'h' => 60 * 60,
                    'd' => 60 * 60 * 24,
                    'w' => 60 * 60 * 24 * 7,
                    _ => {
                        return Err(crate::InputError::Message(format!(
                            "Unknown time specifier {value}."
                        )));
                    }
                };
                total += current * multiplier;
                current = 0;
            } else {
                return Err(crate::InputError::Message(format!(
                    "Unknown time specifier {value}."
                )));
            }
        }

        Ok(total)
    }

    /// Parses CLI arguments into an [`Options`] value and a parse status.
    ///
    /// # Errors
    ///
    /// Returns an error when command-line values cannot be parsed or help/version
    /// output cannot be written.
    pub fn parse(args: &[String], header: &str) -> crate::Result<(OptionsParseStatus, Self)> {
        Self::parse_base(args, true, header)
    }

    fn parse_base(args: &[String], new_opt: bool, header: &str) -> crate::Result<(OptionsParseStatus, Self)> {
        let mut command = Command::new("wassign")
            .disable_help_flag(true)
            .disable_version_flag(true)
            .arg(
                Arg::new("help")
                    .short('h')
                    .long("help")
                    .help("Show this help.")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("version")
                    .long("version")
                    .help("Show version.")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("input")
                    .short('i')
                    .long("input")
                    .help("Specifies an input file.")
                    .action(ArgAction::Append)
                    .num_args(1),
            )
            .arg(
                Arg::new("output")
                    .short('o')
                    .long("output")
                    .help("Specifies an output file.")
                    .num_args(1),
            )
            .arg(
                Arg::new("any")
                    .short('a')
                    .long("any")
                    .help("Stop after the first found solution.")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("pref-exp")
                    .short('p')
                    .long("pref-exp")
                    .help("The preference exponent.")
                    .num_args(1),
            )
            .arg(
                Arg::new("timeout")
                    .short('t')
                    .long("timeout")
                    .help("Sets the optimization timeout.")
                    .num_args(1),
            )
            .arg(
                Arg::new("cs-timeout")
                    .long("cs-timeout")
                    .help("Sets the timeout for attempting to satisfy critical sets of a certain preference level.")
                    .num_args(1),
            )
            .arg(
                Arg::new("no-cs")
                    .long("no-cs")
                    .help("Do not perform critical set analysis.")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("no-cs-simp")
                    .long("no-cs-simp")
                    .help("Do not perform critical set simplification (only relevant if critical set analysis is enabled).")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("threads")
                    .short('j')
                    .long("threads")
                    .help("Number of threads to use for computation.")
                    .num_args(1),
            )
            .arg(
                Arg::new("max-neighbors")
                    .short('n')
                    .long("max-neighbors")
                    .help("Maximum number of neighbor schedulings that will be explored per hill climbing iteration.")
                    .num_args(1),
            )
            .arg(
                Arg::new("greedy")
                    .short('g')
                    .long("greedy")
                    .help("Use sum-based scoring only instead of worst-preference-first scoring.")
                    .action(ArgAction::SetTrue),
            );

        let matches = match command.clone().try_get_matches_from(args) {
            Ok(matches) => matches,
            Err(err) if err.kind() == ErrorKind::DisplayHelp => {
                if new_opt {
                    print!("{header}\n{}", command.render_long_help());
                    return Ok((OptionsParseStatus::Exit, Self::default()));
                }
                return Ok((OptionsParseStatus::Ok, Self::default()));
            }
            Err(err) if err.kind() == ErrorKind::DisplayVersion => {
                if new_opt {
                    println!("{}", crate::version::WASSIGN_VERSION);
                    return Ok((OptionsParseStatus::Exit, Self::default()));
                }
                return Ok((OptionsParseStatus::Ok, Self::default()));
            }
            Err(err) => {
                if new_opt {
                    err.print()
                        .map_err(|print_err| crate::InputError::Message(print_err.to_string()))?;
                }
                return Ok((OptionsParseStatus::Error, Self::default()));
            }
        };

        if new_opt && matches.get_flag("help") {
            print!("{header}\n{}", command.render_long_help());
            return Ok((OptionsParseStatus::Exit, Self::default()));
        }
        if new_opt && matches.get_flag("version") {
            println!("{}", crate::version::WASSIGN_VERSION);
            return Ok((OptionsParseStatus::Exit, Self::default()));
        }

        let mut parsed = Self::default();
        parsed.input_files = matches
            .get_many::<String>("input")
            .map(|values| values.cloned().collect())
            .unwrap_or_default();
        parsed.output_file = matches
            .get_one::<String>("output")
            .cloned()
            .unwrap_or_default();
        parsed.any = matches.get_flag("any");
        parsed.preference_exponent = matches
            .get_one::<String>("pref-exp")
            .map_or(Ok(parsed.preference_exponent), |value| {
                value
                    .parse::<f64>()
                    .map_err(|err| crate::InputError::Message(err.to_string()))
            })?;
        if let Some(value) = matches.get_one::<String>("timeout") {
            parsed.timeout_seconds = Self::parse_time(value)?;
        }
        if let Some(value) = matches.get_one::<String>("cs-timeout") {
            parsed.critical_set_timeout_seconds = Self::parse_time(value)?;
        }
        parsed.no_critical_sets = matches.get_flag("no-cs");
        parsed.no_critical_set_simplification = matches.get_flag("no-cs-simp");
        parsed.thread_count = matches
            .get_one::<String>("threads")
            .map_or(Ok(parsed.thread_count), |value| {
                value
                    .parse::<i32>()
                    .map_err(|err| crate::InputError::Message(err.to_string()))
            })?;
        parsed.max_neighbors = matches
            .get_one::<String>("max-neighbors")
            .map_or(Ok(parsed.max_neighbors), |value| {
                value
                    .parse::<i32>()
                    .map_err(|err| crate::InputError::Message(err.to_string()))
            })?;
        parsed.greedy = matches.get_flag("greedy");

        if new_opt {
            eprintln!("{header}");
        }

        Ok((OptionsParseStatus::Ok, parsed))
    }
}
