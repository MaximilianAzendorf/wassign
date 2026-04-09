//! Command-line entry point for `wassign`.
#![expect(
    clippy::cast_possible_truncation,
    clippy::cast_precision_loss,
    reason = "the overflow pre-check intentionally uses floating-point approximation"
)]

use std::io::Read;
use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};

use wassign::{
    CriticalSetAnalysis, InputReader, MipFlowStaticData, Options, OptionsParseStatus, OutputFormatter, Rng, Scoring,
    ShotgunSolverThreaded, Status,
};

fn main() {
    std::process::exit(run());
}

fn run() -> i32 {
    let seed = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos() as u64;
    Rng::seed(seed);

    let header = format!(
        "{} [Version {}]\n(c) 2022 {}\n",
        clap::crate_name!(),
        wassign::WASSIGN_VERSION,
        clap::crate_authors!(),
    );
    let args = std::env::args().collect::<Vec<_>>();

    let (parse_status, options) = match Options::parse(&args, &header) {
        Ok(result) => result,
        Err(err) => {
            eprintln!("{err}");
            return 1;
        }
    };
    let options = Arc::new(options);
    Status::enable_output(&options);
    Status::debug(&format!("Parsed options: {options:?}"));

    match parse_status {
        OptionsParseStatus::Ok => {}
        OptionsParseStatus::Exit => return 0,
        OptionsParseStatus::Error => {
            Status::error("Invalid arguments.");
            return 1;
        }
    }

    let input_string = match read_input_string(&options) {
        Ok(input) => input,
        Err(err) => {
            Status::error(&format!("Error in input: {err}"));
            return 1;
        }
    };

    let result = try_run(&input_string, &options);
    match result {
        Ok(()) => 0,
        Err(err) => {
            Status::error(&format!("Error in input: {err}"));
            1
        }
    }
}

fn try_run(input_string: &str, options: &Arc<Options>) -> wassign::Result<()> {
    Status::info("Processing input.");
    Status::debug(&format!("Input size: {} byte(s).", input_string.len()));
    let mut reader = InputReader::new(options);
    let input_data = reader.read_input(input_string)?;
    let options = reader.effective_options();

    Status::info(&format!(
        "Read {} slot(s), {} choice(s) and {} chooser(s).",
        input_data.slot_count(),
        input_data.choice_count(),
        input_data.chooser_count()
    ));
    Status::info(&format!(
        "Found {} scheduling and {} assignment constraints.",
        input_data.scheduling_constraints().len(),
        input_data.assignment_constraints().len()
    ));

    if (f64::from(input_data.max_preference())).powf(options.preference_exponent)
        * (input_data.chooser_count() as f64)
        >= (i64::MAX as f64)
    {
        Status::warning("The preference exponent is too large; computations may cause an integer overflow");
    }

    let scoring = Arc::new(Scoring::new(input_data.clone(), options.clone()));
    let do_cs_analysis = !options.no_critical_sets && !options.greedy && input_data.slot_count() > 1;
    let do_cs_simplification = do_cs_analysis && !options.no_critical_set_simplification;
    Status::debug(&format!(
        "Critical-set analysis enabled: {do_cs_analysis}; simplification enabled: {do_cs_simplification}."
    ));
    Status::info(if do_cs_analysis {
        if do_cs_simplification {
            "Performing critical set analysis with simplification."
        } else {
            "Performing critical set analysis without simplification."
        }
    } else {
        "Skipping critical set analysis."
    });

    let cs_analysis = Arc::new(CriticalSetAnalysis::new(
        input_data.clone(),
        do_cs_analysis,
        do_cs_simplification,
    ));
    if do_cs_analysis {
        Status::info(&format!(
            "Critical set analysis gives a preference bound of {}.",
            cs_analysis.preference_bound()
        ));
    }

    Status::info("Generating static data and starting solver.");
    let static_data = Arc::new(MipFlowStaticData::new(input_data.as_ref()));
    Status::debug("Static flow data generated.");
    let mut solver = ShotgunSolverThreaded::new(
        input_data.clone(),
        cs_analysis,
        static_data,
        scoring.clone(),
        options.clone(),
    );
    solver.start()?;
    let solution = Status::track_solver(&mut solver)?;

    if solution.is_invalid() {
        Status::info_important("No solution found.");
        return Ok(());
    }

    Status::info_important("Solution found.");
    Status::info(&format!("Solution score: {}", scoring.evaluate(&solution).to_str()));

    if input_data.slot_count() > 1 {
        let scheduling = OutputFormatter::write_scheduling_solution(&solution)?;
        output_string(scheduling.as_str(), ".scheduling.csv", &options)?;
    }
    let assignment = OutputFormatter::write_assignment_solution(&solution)?;
    output_string(assignment.as_str(), ".assignment.csv", &options)?;

    Ok(())
}

fn read_input_string(options: &Options) -> wassign::Result<String> {
    if options.input_files.is_empty() {
        Status::debug("Reading input from standard input.");
        let mut input = String::new();
        std::io::stdin()
            .read_to_string(&mut input)
            .map_err(|err| wassign::InputError::Message(err.to_string()))?;
        return Ok(input);
    }

    let mut result = String::new();
    for file in &options.input_files {
        Status::debug(&format!("Reading input file `{file}`."));
        let content =
            std::fs::read_to_string(file).map_err(|err| wassign::InputError::Message(err.to_string()))?;
        result.push_str(&content);
        result.push('\n');
    }
    Ok(result)
}

fn output_string(text: &str, file_suffix: &str, options: &Options) -> wassign::Result<()> {
    if options.output_file.is_empty() {
        println!("{text}\n");
    } else {
        let output_path = format!("{}{}", options.output_file, file_suffix);
        Status::info(&format!("Writing output to `{output_path}`."));
        std::fs::write(output_path, text)
            .map_err(|err| wassign::InputError::Message(err.to_string()))?;
    }
    Ok(())
}
