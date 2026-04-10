//! Command-line entry point for `wassign`.

use std::io::Read;
use std::time::{SystemTime, UNIX_EPOCH};

use clap::Parser;
use wassign::{
    InputReader, Options, OutputFormatter, PreparedProblem, Rng, Solution, ThreadedSolver, status,
};

fn main() {
    std::process::exit(run());
}

fn run() -> i32 {
    let seed = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos()
        .try_into()
        .unwrap_or_default();
    Rng::seed(seed);

    let options = Options::parse();
    status::enable_output(&options);
    status::debug(&format!("Parsed options: {options:?}"));

    let input_string = match read_input_string(&options) {
        Ok(input) => input,
        Err(err) => {
            status::error(&format!("Error in input: {err}"));
            return 1;
        }
    };

    let result = try_run(&input_string, &options);
    match result {
        Ok(()) => 0,
        Err(err) => {
            status::error(&format!("Error in input: {err}"));
            1
        }
    }
}

fn try_run(input_string: &str, options: &Options) -> wassign::Result<()> {
    status::info("Processing input.");
    status::debug(&format!("Input size: {} byte(s).", input_string.len()));
    let mut reader = InputReader::new(options);
    let input_data = reader.read_input(input_string)?;
    let options = reader.effective_options();

    status::info(&format!(
        "Read {} slot(s), {} choice(s) and {} chooser(s).",
        input_data.slots.len(),
        input_data.choices.len(),
        input_data.choosers.len()
    ));
    status::info(&format!(
        "Found {} scheduling and {} assignment constraints.",
        input_data.scheduling_constraints.len(),
        input_data.assignment_constraints.len()
    ));

    let chooser_count = u32::try_from(input_data.choosers.len()).unwrap_or(u32::MAX);
    if f64::from(input_data.max_preference).powf(options.preference_exponent)
        * f64::from(chooser_count)
        >= 2_f64.powi(63) - 1.0
    {
        status::warning(
            "The preference exponent is too large; computations may cause an integer overflow",
        );
    }

    let do_cs_analysis = !options.no_critical_sets && !options.greedy && input_data.slots.len() > 1;
    let do_cs_simplification = do_cs_analysis && !options.no_critical_set_simplification;
    status::debug(&format!(
        "Critical-set analysis enabled: {do_cs_analysis}; simplification enabled: {do_cs_simplification}."
    ));
    status::info(if do_cs_analysis {
        if do_cs_simplification {
            "Performing critical set analysis with simplification."
        } else {
            "Performing critical set analysis without simplification."
        }
    } else {
        "Skipping critical set analysis."
    });

    let problem = PreparedProblem::new(input_data, &options);
    if do_cs_analysis {
        status::info(&format!(
            "Critical set analysis gives a preference bound of {}.",
            problem.critical_set_analysis.preference_bound()
        ));
    }

    status::info("Generating static data and starting solver.");
    let solver = ThreadedSolver::new(problem, options.clone());
    let running = solver.start()?;
    let result = status::track_solver(running)?;
    let input_data = &result.input_data;
    let solution = &result.solution;

    if *solution == Solution::Invalid {
        status::info_important("No solution found.");
        return Ok(());
    }

    status::info_important("Solution found.");
    status::info(&format!(
        "Solution score: {}",
        result.scoring.evaluate(input_data, solution).to_str()
    ));

    if input_data.slots.len() > 1 {
        let scheduling = OutputFormatter::write_scheduling_solution(input_data, solution)?;
        output_string(scheduling.as_str(), ".scheduling.csv", &options)?;
    }
    let assignment = OutputFormatter::write_assignment_solution(input_data, solution)?;
    output_string(assignment.as_str(), ".assignment.csv", &options)?;

    Ok(())
}

fn read_input_string(options: &Options) -> wassign::Result<String> {
    if options.input_files.is_empty() {
        status::debug("Reading input from standard input.");
        let mut input = String::new();
        std::io::stdin()
            .read_to_string(&mut input)
            .map_err(|err| wassign::InputError::Message(err.to_string()))?;
        return Ok(input);
    }

    let mut result = String::new();
    for file in &options.input_files {
        status::debug(&format!("Reading input file `{file}`."));
        let content = std::fs::read_to_string(file)
            .map_err(|err| wassign::InputError::Message(err.to_string()))?;
        result.push_str(&content);
        result.push('\n');
    }
    Ok(result)
}

fn output_string(text: &str, file_suffix: &str, options: &Options) -> wassign::Result<()> {
    if let Some(output_file) = &options.output_file {
        let output_path = format!("{output_file}{file_suffix}");
        status::info(&format!("Writing output to `{output_path}`."));
        std::fs::write(output_path, text)
            .map_err(|err| wassign::InputError::Message(err.to_string()))?;
    } else {
        println!("{text}\n");
    }
    Ok(())
}
