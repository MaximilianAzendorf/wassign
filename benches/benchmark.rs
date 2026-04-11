//! Configurable benchmark runner for repeated solver evaluation and comparison.

use std::env;
use std::ffi::OsStr;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

use clap::Parser;
use serde::{Deserialize, Serialize};
use wassign::{InputReader, Options, PreparedProblem, Rng, ThreadedSolver};

const CHILD_ENV: &str = "WASSIGN_BENCH_CHILD";
const RESULTS_ENV: &str = "WASSIGN_BENCH_RESULTS_PATH";

#[derive(Debug, Deserialize)]
struct BenchmarkConfig {
    #[serde(rename = "N", alias = "n")]
    n: usize,
    runs: Vec<String>,
    compare_commit: Option<String>,
}

#[derive(Debug)]
struct PreparedRun {
    display_args: String,
    options: Options,
    problem: PreparedProblem,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct BenchmarkResult {
    average_major: Option<f64>,
    average_minor: Option<f64>,
    solved_runs: usize,
    no_solution_runs: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct RunResult {
    display_args: String,
    result: BenchmarkResult,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct BenchmarkReport {
    checkout_label: String,
    runs: Vec<RunResult>,
}

struct TemporaryWorktree {
    repo_root: PathBuf,
    path: PathBuf,
}

impl Drop for TemporaryWorktree {
    fn drop(&mut self) {
        let _ = Command::new("git")
            .arg("-C")
            .arg(&self.repo_root)
            .args(["worktree", "remove", "--force"])
            .arg(&self.path)
            .status();
        let _ = fs::remove_dir_all(&self.path);
    }
}

fn main() {
    if let Err(err) = run() {
        eprintln!("{err}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let config_path = benchmark_config_path()?;
    if env::var_os(CHILD_ENV).is_some() {
        let report = run_local_report(&config_path, "comparison checkout".to_owned())?;
        write_child_report(&report)?;
        return Ok(());
    }

    let current_report = run_local_report(&config_path, "current checkout".to_owned())?;
    let compare_commit = read_config(&config_path)?
        .compare_commit
        .unwrap_or_else(|| "HEAD".to_owned());
    let compare_commit = resolve_compare_commit(&compare_commit)?;
    let reference_report = run_reference_report(&config_path, &compare_commit)?;

    print_comparison_summary(&current_report, &reference_report);
    Ok(())
}

fn run_local_report(config_path: &Path, checkout_label: String) -> Result<BenchmarkReport, String> {
    let config = read_config(config_path)?;
    let prepared_runs = config
        .runs
        .iter()
        .map(|run_spec| prepare_run(run_spec, config_path))
        .collect::<Result<Vec<_>, _>>()?;

    let base_seed = current_seed();
    let mut runs = Vec::with_capacity(prepared_runs.len());

    for (run_index, prepared_run) in prepared_runs.iter().enumerate() {
        println!(
            "=== Benchmarking {} in {} ===",
            prepared_run.display_args, checkout_label
        );
        let result = run_benchmark(
            prepared_run,
            config.n,
            base_seed.wrapping_add(run_index as u64),
        );
        println!(
            "Average best found solution score over {} runs for {} in {}: major={}, minor={} (solved={}, no-solution={})",
            config.n,
            prepared_run.display_args,
            checkout_label,
            format_score_term(result.average_major),
            format_score_term(result.average_minor),
            result.solved_runs,
            result.no_solution_runs,
        );
        runs.push(RunResult {
            display_args: prepared_run.display_args.clone(),
            result,
        });
    }

    Ok(BenchmarkReport {
        checkout_label,
        runs,
    })
}

fn run_reference_report(config_path: &Path, compare_commit: &str) -> Result<BenchmarkReport, String> {
    let repo_root = repo_root()?;
    let worktree = create_temporary_worktree(&repo_root, compare_commit)?;
    let results_path = env::temp_dir().join(format!("wassign-benchmark-results-{}.json", unique_id()));

    let status = Command::new("cargo")
        .current_dir(&worktree.path)
        .args(["bench", "--bench", "benchmark", "--"])
        .arg(config_path)
        .env(CHILD_ENV, "1")
        .env(RESULTS_ENV, &results_path)
        .status()
        .map_err(|err| format!("Could not run benchmark in worktree {}: {err}", worktree.path.display()))?;

    if !status.success() {
        return Err(format!(
            "Benchmark failed in comparison worktree at commit {compare_commit}"
        ));
    }

    let text = fs::read_to_string(&results_path)
        .map_err(|err| format!("Could not read comparison results {}: {err}", results_path.display()))?;
    let mut report: BenchmarkReport = serde_json::from_str(&text)
        .map_err(|err| format!("Could not parse comparison results {}: {err}", results_path.display()))?;
    report.checkout_label = format!("comparison checkout ({compare_commit})");
    let _ = fs::remove_file(results_path);
    Ok(report)
}

fn print_comparison_summary(current_report: &BenchmarkReport, reference_report: &BenchmarkReport) {
    println!("=== Summary ===");
    for (current_run, reference_run) in current_report.runs.iter().zip(&reference_report.runs) {
        println!(
            "{}: {} major={}, minor={}, solved={}, no-solution={}; {} major={}, minor={}, solved={}, no-solution={}",
            current_run.display_args,
            current_report.checkout_label,
            format_score_term(current_run.result.average_major),
            format_score_term(current_run.result.average_minor),
            current_run.result.solved_runs,
            current_run.result.no_solution_runs,
            reference_report.checkout_label,
            format_score_term(reference_run.result.average_major),
            format_score_term(reference_run.result.average_minor),
            reference_run.result.solved_runs,
            reference_run.result.no_solution_runs,
        );
    }
}

fn benchmark_config_path() -> Result<PathBuf, String> {
    let mut args = env::args_os().skip(1).filter(|arg| arg != "--bench");
    match args.next() {
        Some(path) => Ok(PathBuf::from(path)),
        None => env::current_dir()
            .map(|cwd| cwd.join("benches").join("benchmark.toml"))
            .map_err(|err| format!("Could not determine current directory: {err}")),
    }
}

fn read_config(path: &Path) -> Result<BenchmarkConfig, String> {
    let text = fs::read_to_string(path)
        .map_err(|err| format!("Could not read benchmark config {}: {err}", path.display()))?;
    toml::from_str(&text)
        .map_err(|err| format!("Could not parse benchmark config {}: {err}", path.display()))
}

fn prepare_run(run_spec: &str, config_path: &Path) -> Result<PreparedRun, String> {
    let display_args = if run_spec.trim().is_empty() {
        "(no args)".to_owned()
    } else {
        run_spec.to_owned()
    };

    let cli_args = shell_words(run_spec).map_err(|err| {
        format!("Could not parse run spec `{display_args}` from {}: {err}", config_path.display())
    })?;
    let options = parse_options(&cli_args)
        .map_err(|err| format!("Invalid run spec `{display_args}`: {err}"))?;
    let input = read_input_string(&options, config_path)?;
    let mut reader = InputReader::new(&options);
    let input_data = reader
        .read_input(&input)
        .map_err(|err| format!("Could not read input for `{display_args}`: {err}"))?;
    let effective_options = reader.effective_options();
    let problem = PreparedProblem::new(input_data, &effective_options);

    Ok(PreparedRun {
        display_args,
        options: effective_options,
        problem,
    })
}

fn parse_options(args: &[String]) -> Result<Options, clap::Error> {
    let cli_args = std::iter::once("benchmark".to_owned())
        .chain(args.iter().cloned())
        .collect::<Vec<_>>();
    Options::try_parse_from(cli_args)
}

fn shell_words(run_spec: &str) -> Result<Vec<String>, String> {
    let mut words = Vec::new();
    let mut current = String::new();
    let mut quote: Option<char> = None;
    let mut escaped = false;

    for ch in run_spec.chars() {
        if escaped {
            current.push(ch);
            escaped = false;
            continue;
        }

        match ch {
            '\\' => escaped = true,
            '\'' | '"' if quote == Some(ch) => quote = None,
            '\'' | '"' if quote.is_none() => quote = Some(ch),
            ch if ch.is_whitespace() && quote.is_none() => {
                if !current.is_empty() {
                    words.push(std::mem::take(&mut current));
                }
            }
            _ => current.push(ch),
        }
    }

    if escaped {
        return Err("unfinished escape sequence".to_owned());
    }
    if quote.is_some() {
        return Err("unterminated quoted string".to_owned());
    }
    if !current.is_empty() {
        words.push(current);
    }
    Ok(words)
}

fn read_input_string(options: &Options, config_path: &Path) -> Result<String, String> {
    if options.input_files.is_empty() {
        return Err(format!(
            "run config in {} must specify at least one --input file",
            config_path.display()
        ));
    }

    let config_dir = config_path
        .parent()
        .map_or_else(|| PathBuf::from("."), Path::to_path_buf);
    let mut result = String::new();
    for file in &options.input_files {
        let path = resolve_input_path(file, &config_dir)?;
        let content = fs::read_to_string(&path)
            .map_err(|err| format!("Could not read input file {}: {err}", path.display()))?;
        result.push_str(&content);
        result.push('\n');
    }
    Ok(result)
}

fn resolve_input_path(file: &str, config_dir: &Path) -> Result<PathBuf, String> {
    let path = PathBuf::from(file);
    if path.is_absolute() {
        return Ok(path);
    }

    let cwd_path = env::current_dir()
        .map_err(|err| format!("Could not determine current directory: {err}"))?
        .join(&path);
    if cwd_path.exists() {
        return Ok(cwd_path);
    }

    let config_relative_path = config_dir.join(path);
    if config_relative_path.exists() {
        return Ok(config_relative_path);
    }

    Err(format!(
        "Could not resolve relative input path `{file}` from current directory or config directory"
    ))
}

fn run_benchmark(prepared_run: &PreparedRun, n: usize, base_seed: u64) -> BenchmarkResult {
    let mut major_sum = 0.0_f64;
    let mut major_count = 0_usize;
    let mut minor_sum = 0.0_f64;
    let mut solved_runs = 0_usize;
    let mut no_solution_runs = 0_usize;

    for iteration in 0..n {
        let seed = base_seed.wrapping_add(iteration as u64);
        Rng::seed(seed);

        let solver = ThreadedSolver::new(prepared_run.problem.clone(), prepared_run.options.clone());
        let result = solver
            .start()
            .and_then(|running| running.wait())
            .unwrap_or_else(|err| panic!("solver failed for {}: {err}", prepared_run.display_args));

        if result.solution.is_invalid() {
            no_solution_runs += 1;
            println!("Run {}/{}: no solution", iteration + 1, n);
            continue;
        }

        let score = result.scoring.evaluate(&result.input_data, &result.solution);
        solved_runs += 1;
        minor_sum += f64::from(score.minor());
        if score.major().is_finite() {
            major_sum += f64::from(score.major());
            major_count += 1;
            println!(
                "Run {}/{}: major={:.0}, minor={:.5}",
                iteration + 1,
                n,
                score.major(),
                score.minor(),
            );
        } else {
            println!("Run {}/{}: minor={:.5}", iteration + 1, n, score.minor());
        }
    }

    BenchmarkResult {
        average_major: if major_count > 0 {
            Some(major_sum / major_count as f64)
        } else {
            None
        },
        average_minor: if solved_runs > 0 {
            Some(minor_sum / solved_runs as f64)
        } else {
            None
        },
        solved_runs,
        no_solution_runs,
    }
}

fn resolve_compare_commit(value: &str) -> Result<String, String> {
    if value == "newest" {
        git_stdout(["rev-parse", "HEAD"])
    } else {
        git_stdout(["rev-parse", value])
    }
}

fn repo_root() -> Result<PathBuf, String> {
    git_stdout(["rev-parse", "--show-toplevel"]).map(PathBuf::from)
}

fn git_stdout<I, S>(args: I) -> Result<String, String>
where
    I: IntoIterator<Item = S>,
    S: AsRef<OsStr>,
{
    let output = Command::new("git")
        .args(args)
        .output()
        .map_err(|err| format!("Could not execute git: {err}"))?;
    if !output.status.success() {
        return Err(String::from_utf8_lossy(&output.stderr).trim().to_owned());
    }
    Ok(String::from_utf8_lossy(&output.stdout).trim().to_owned())
}

fn create_temporary_worktree(repo_root: &Path, compare_commit: &str) -> Result<TemporaryWorktree, String> {
    let path = env::temp_dir().join(format!("wassign-benchmark-worktree-{}", unique_id()));
    let status = Command::new("git")
        .arg("-C")
        .arg(repo_root)
        .args(["worktree", "add", "--detach"])
        .arg(&path)
        .arg(compare_commit)
        .status()
        .map_err(|err| format!("Could not create git worktree: {err}"))?;
    if !status.success() {
        return Err(format!(
            "git worktree add failed for commit {compare_commit}"
        ));
    }

    Ok(TemporaryWorktree {
        repo_root: repo_root.to_path_buf(),
        path,
    })
}

fn write_child_report(report: &BenchmarkReport) -> Result<(), String> {
    let results_path = env::var_os(RESULTS_ENV)
        .map(PathBuf::from)
        .ok_or_else(|| format!("Missing {RESULTS_ENV} environment variable"))?;
    let text = serde_json::to_string(report)
        .map_err(|err| format!("Could not serialize benchmark report: {err}"))?;
    fs::write(&results_path, text)
        .map_err(|err| format!("Could not write benchmark report {}: {err}", results_path.display()))
}

fn current_seed() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos()
        .try_into()
        .unwrap_or_default()
}

fn unique_id() -> u64 {
    current_seed()
}

fn format_score_term(value: Option<f64>) -> String {
    value.map_or_else(|| "n/a".to_owned(), |value| format!("{value:.5}"))
}
