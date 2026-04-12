use clap::Parser;

#[derive(Debug, Clone, Parser)]
/// Command-line options for the `wassign` executable.
#[command(name = "wassign", author, version, about, long_about = None)]
pub struct Options {
    /// Input files to read. When empty, input is read from standard input.
    #[arg(short = 'i', long = "input")]
    pub input_files: Vec<String>,
    /// Output path prefix used for generated CSV files.
    #[arg(short = 'o', long = "output")]
    pub output_file: Option<String>,
    /// Whether any valid solution is acceptable.
    #[arg(short = 'a', long = "any")]
    pub any: bool,
    /// Exponent used when converting preferences into assignment costs.
    #[arg(short = 'p', long = "pref-exp", default_value_t = 3.0)]
    pub preference_exponent: f64,
    /// Overall solver timeout in seconds.
    #[arg(short = 't', long = "timeout", value_parser = Self::parse_time, default_value = "60s")]
    pub timeout_seconds: i32,
    /// Time to spend with the current critical-set preference cutoff before relaxing it.
    #[arg(long = "cs-timeout", value_parser = Self::parse_time, default_value = "3s")]
    pub critical_set_timeout_seconds: i32,
    /// Disables critical-set analysis.
    #[arg(long = "no-cs")]
    pub no_critical_sets: bool,
    /// Disables simplification during critical-set analysis.
    #[arg(long = "no-cs-simp")]
    pub no_critical_set_simplification: bool,
    /// Number of worker threads to use.
    #[arg(
        short = 'j',
        long = "threads",
        default_value_t = default_thread_count()
    )]
    pub thread_count: u32,
    /// Maximum number of hill-climbing neighbors to inspect per step.
    #[arg(short = 'n', long = "max-neighbors", default_value_t = 12)]
    pub max_neighbors: u32,
    /// Enables greedy assignment solving.
    #[arg(short = 'g', long = "greedy")]
    pub greedy: bool,
    /// Emits machine-readable progress events to standard output.
    #[arg(long = "progress-stream", hide = true)]
    pub progress_stream: bool,
}

impl Default for Options {
    fn default() -> Self {
        Self {
            input_files: Vec::new(),
            output_file: None,
            any: false,
            preference_exponent: 3.0,
            timeout_seconds: 60,
            critical_set_timeout_seconds: 3,
            no_critical_sets: false,
            no_critical_set_simplification: false,
            thread_count: default_thread_count(),
            max_neighbors: 12,
            greedy: false,
            progress_stream: false,
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
                    .ok_or_else(|| {
                        crate::InputError::Message(format!("Unknown time specifier {value}."))
                    })?;
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
}

fn default_thread_count() -> u32 {
    std::thread::available_parallelism()
        .map_or(1_u32, |value| u32::try_from(value.get()).unwrap_or(1))
}
