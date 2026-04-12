//! Terminal benchmark dashboard for comparing the current checkout against a
//! compatible comparison commit.

use std::ffi::OsStr;
use std::fs;
use std::io::{BufRead, BufReader, IsTerminal};
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::sync::{
    Arc, Mutex,
    atomic::{AtomicBool, Ordering},
    mpsc::{self, Receiver, RecvTimeoutError, Sender},
};
use std::thread::JoinHandle;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

#[cfg(unix)]
use std::os::unix::process::CommandExt;

use clap::{Parser, ValueEnum};
use console::style;
use crossterm::event::{self, Event as CrosstermEvent, KeyCode, KeyModifiers};
use crossterm::execute;
use crossterm::terminal::{
    EnterAlternateScreen, LeaveAlternateScreen, disable_raw_mode, enable_raw_mode,
};
use ratatui::backend::CrosstermBackend;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Modifier, Style};
use ratatui::text::Line;
use ratatui::widgets::{Block, Borders, Cell, Paragraph, Row, Table, Wrap};
use ratatui::{Frame, Terminal};
use wassign::{ProgressStreamEvent, Score, ThreadedSolverProgress};

const UI_TICK: Duration = Duration::from_millis(100);
const PLAIN_PROGRESS_INTERVAL: Duration = Duration::from_mins(1);
const MIN_RUNNER_PANEL_HEIGHT: u16 = 15;
const MAX_UI_EVENTS_PER_FRAME: usize = 1024;
const WORKTREE_PREFIX: &str = "wassign-benchmark-worktree-";

#[derive(Debug, serde::Deserialize)]
struct BenchmarkConfig {
    iterations: usize,
    runners: usize,
    runs: Vec<String>,
    compare_commit: Option<String>,
}

#[derive(Debug, Parser)]
#[command(author, version, about, long_about = None)]
struct Cli {
    /// Benchmark config TOML path.
    config_path: Option<PathBuf>,
    /// Only run one side of the comparison. Omit to run both.
    #[arg(long, value_enum, value_name = "TARGET")]
    only: Option<BenchmarkOnly>,
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum BenchmarkOnly {
    Working,
    Commit,
}

#[derive(Debug, Clone, Copy)]
enum BenchmarkTarget {
    Both,
    Working,
    Commit,
}

impl From<Option<BenchmarkOnly>> for BenchmarkTarget {
    fn from(value: Option<BenchmarkOnly>) -> Self {
        match value {
            Some(BenchmarkOnly::Working) => Self::Working,
            Some(BenchmarkOnly::Commit) => Self::Commit,
            None => Self::Both,
        }
    }
}

impl BenchmarkTarget {
    fn runs_working(self) -> bool {
        matches!(self, Self::Both | Self::Working)
    }

    fn runs_commit(self) -> bool {
        matches!(self, Self::Both | Self::Commit)
    }

    fn display(self) -> &'static str {
        match self {
            Self::Both => "both",
            Self::Working => "working",
            Self::Commit => "commit",
        }
    }
}

#[derive(Debug, Clone)]
struct RunConfig {
    display_args: String,
    cli_args: Vec<String>,
}

#[derive(Debug, Clone, Default)]
struct BenchmarkResult {
    major_sum: f64,
    major_count: usize,
    minor_sum: f64,
    solved_runs: usize,
    no_solution_runs: usize,
}

#[derive(Debug, Clone, Copy)]
enum CheckoutKind {
    Working,
    Comparison,
}

#[derive(Debug)]
enum UiEvent {
    RunnerStarted {
        runner: usize,
        assigned_iterations: usize,
    },
    IterationStarted {
        runner: usize,
        iteration: usize,
    },
    Progress {
        runner: usize,
        progress: ThreadedSolverProgress,
    },
    IterationComplete {
        runner: usize,
        score: Option<Score>,
    },
    RunnerFinished {
        runner: usize,
    },
}

struct TemporaryWorktree {
    path: PathBuf,
    cleaner: WorktreeCleaner,
}

impl Drop for TemporaryWorktree {
    fn drop(&mut self) {
        self.cleaner.cleanup_path(&self.path);
    }
}

#[derive(Clone)]
struct WorktreeCleaner {
    repo_root: PathBuf,
    paths: Arc<Mutex<Vec<PathBuf>>>,
}

struct AppState {
    run_labels: Vec<String>,
    compare_short_commit: String,
    working_results: Vec<Option<BenchmarkResult>>,
    comparison_results: Vec<Option<BenchmarkResult>>,
    runner_states: Vec<RunnerPaneState>,
    phase_status: String,
}

#[derive(Debug, Clone)]
struct RunnerPaneState {
    title: String,
    status: RunnerStatus,
    assigned_iterations: usize,
    current_iteration: Option<usize>,
    completed_iterations: usize,
    latest_progress: Option<ThreadedSolverProgress>,
    last_result: Option<String>,
}

#[derive(Debug, Clone, Copy)]
enum RunnerStatus {
    Waiting,
    Running,
    Finished,
}

struct PlainPhaseProgress {
    next_report: Instant,
}

type BenchTerminal = Terminal<CrosstermBackend<std::io::Stdout>>;

fn main() {
    if let Err(err) = run() {
        eprintln!("{err}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let cli = Cli::parse();
    let target = BenchmarkTarget::from(cli.only);
    let config_path = benchmark_config_path(cli.config_path);
    let config = read_config(&config_path)?;
    let runs = config
        .runs
        .iter()
        .map(|run_spec| prepare_run(run_spec))
        .collect::<Result<Vec<_>, _>>()?;
    let workspace_root = workspace_root()?;
    cleanup_stale_benchmark_worktrees(&workspace_root)?;
    let cleaner = WorktreeCleaner::new(workspace_root.clone());
    cleaner.install_ctrlc_handler()?;
    let compare_commit =
        resolve_compare_commit(config.compare_commit.as_deref().unwrap_or("HEAD"))?;
    let working_short_commit = git_stdout(["rev-parse", "--short", "HEAD"])?;
    let compare_short_commit = git_stdout(["rev-parse", "--short", compare_commit.as_str()])?;
    let worktree = if target.runs_commit() {
        let worktree =
            create_temporary_worktree(&workspace_root, &compare_commit, cleaner.clone())?;
        ensure_supported_comparison_checkout(&worktree.path)?;
        Some(worktree)
    } else {
        None
    };
    if target.runs_working() {
        build_release_binary(&workspace_root, "working checkout")?;
    }
    if let Some(worktree) = &worktree {
        build_release_binary(
            &worktree.path,
            &format!("comparison checkout {compare_short_commit}"),
        )?;
    }

    let mut app = AppState::new(
        runs.iter().map(|run| run.display_args.clone()).collect(),
        compare_short_commit.clone(),
        config.runners,
    );

    if is_interactive_terminal() {
        let mut terminal = init_terminal()?;
        let run_result = run_dashboard(
            &mut terminal,
            &mut app,
            &workspace_root,
            worktree.as_ref().map(|worktree| worktree.path.as_path()),
            &runs,
            config.iterations,
            config.runners,
            target,
        );
        restore_terminal(&mut terminal)?;
        run_result?;
    } else {
        print_plain_metadata(
            &config_path,
            &working_short_commit,
            &compare_short_commit,
            config.iterations,
            config.runners,
            &runs,
            target,
        );
        run_plain_dashboard(
            &mut app,
            &workspace_root,
            worktree.as_ref().map(|worktree| worktree.path.as_path()),
            &runs,
            config.iterations,
            config.runners,
            target,
        )?;
    }

    print_final_summary(&runs, &app);
    Ok(())
}

fn print_plain_metadata(
    config_path: &Path,
    working_short_commit: &str,
    compare_short_commit: &str,
    iterations: usize,
    runners: usize,
    runs: &[RunConfig],
    target: BenchmarkTarget,
) {
    println!("Benchmark mode: non-interactive");
    println!("Config: {}", config_path.display());
    println!("Target: {}", target.display());
    println!("Working commit: {working_short_commit}");
    println!("Comparison commit: {compare_short_commit}");
    println!("Iterations: {iterations}");
    println!("Runners: {runners}");
    println!("Runs: {}", runs.len());
}

fn is_interactive_terminal() -> bool {
    std::io::stdin().is_terminal() && std::io::stdout().is_terminal()
}

fn run_dashboard(
    terminal: &mut BenchTerminal,
    app: &mut AppState,
    workspace_root: &Path,
    comparison_root: Option<&Path>,
    runs: &[RunConfig],
    iterations: usize,
    runners: usize,
    target: BenchmarkTarget,
) -> Result<(), String> {
    let base_seed = current_seed();
    for (run_index, run) in runs.iter().enumerate() {
        let run_seed = base_seed.wrapping_add((run_index as u64).wrapping_mul(1_000_000));
        if target.runs_working() {
            let working_result = run_phase(
                terminal,
                app,
                run_index,
                CheckoutKind::Working,
                "working",
                workspace_root,
                run,
                iterations,
                runners,
                run_seed,
            )?;
            app.working_results[run_index] = Some(working_result);
        }

        if target.runs_commit() {
            let comparison_root =
                comparison_root.ok_or_else(|| "comparison worktree was not created".to_owned())?;
            let comparison_result = run_phase(
                terminal,
                app,
                run_index,
                CheckoutKind::Comparison,
                &app.compare_short_commit.clone(),
                comparison_root,
                run,
                iterations,
                runners,
                run_seed,
            )?;
            app.comparison_results[run_index] = Some(comparison_result);
        }
    }

    app.phase_status = "Benchmark complete | press q, Esc, or Ctrl+C to exit".to_owned();
    wait_for_exit(terminal, app)?;
    Ok(())
}

fn run_plain_dashboard(
    app: &mut AppState,
    workspace_root: &Path,
    comparison_root: Option<&Path>,
    runs: &[RunConfig],
    iterations: usize,
    runners: usize,
    target: BenchmarkTarget,
) -> Result<(), String> {
    let base_seed = current_seed();
    for (run_index, run) in runs.iter().enumerate() {
        let run_seed = base_seed.wrapping_add((run_index as u64).wrapping_mul(1_000_000));
        if target.runs_working() {
            let working_result = run_plain_phase(
                app,
                run_index,
                runs.len(),
                CheckoutKind::Working,
                "working",
                workspace_root,
                run,
                iterations,
                runners,
                run_seed,
            )?;
            app.working_results[run_index] = Some(working_result);
        }

        if target.runs_commit() {
            let comparison_root =
                comparison_root.ok_or_else(|| "comparison worktree was not created".to_owned())?;
            let comparison_result = run_plain_phase(
                app,
                run_index,
                runs.len(),
                CheckoutKind::Comparison,
                &app.compare_short_commit.clone(),
                comparison_root,
                run,
                iterations,
                runners,
                run_seed,
            )?;
            app.comparison_results[run_index] = Some(comparison_result);
        }
    }

    app.phase_status = "Benchmark complete".to_owned();
    Ok(())
}

fn wait_for_exit(terminal: &mut BenchTerminal, app: &AppState) -> Result<(), String> {
    loop {
        draw_app(terminal, app)?;
        if handle_input()? {
            break;
        }
        std::thread::sleep(UI_TICK);
    }
    draw_app(terminal, app)?;
    Ok(())
}

fn run_phase(
    terminal: &mut BenchTerminal,
    app: &mut AppState,
    run_index: usize,
    checkout_kind: CheckoutKind,
    checkout_label: &str,
    root: &Path,
    run: &RunConfig,
    iterations: usize,
    runners: usize,
    base_seed: u64,
) -> Result<BenchmarkResult, String> {
    app.start_phase(run_index, checkout_label);
    draw_app(terminal, app)?;

    let (tx, rx) = mpsc::channel();
    let cancel = Arc::new(AtomicBool::new(false));
    let handles = spawn_runner_threads(
        root.to_path_buf(),
        run.clone(),
        iterations,
        runners,
        base_seed,
        cancel.clone(),
        tx,
    );

    let mut result = BenchmarkResult::default();
    let mut finished_runners = 0;
    let mut channel_disconnected = false;
    let mut interrupted = false;
    let mut next_draw = Instant::now() + UI_TICK;
    while finished_runners < runners {
        if handle_input()? {
            cancel.store(true, Ordering::Relaxed);
            app.phase_status = "Benchmark interrupted".to_owned();
            interrupted = true;
            break;
        }

        match rx.recv_timeout(next_draw.saturating_duration_since(Instant::now())) {
            Ok(event) => process_ui_event(
                event,
                app,
                &mut result,
                &mut finished_runners,
                checkout_kind,
                run_index,
            ),
            Err(RecvTimeoutError::Timeout) => {}
            Err(RecvTimeoutError::Disconnected) => {
                channel_disconnected = true;
                break;
            }
        }

        drain_ui_events(
            &rx,
            app,
            &mut result,
            &mut finished_runners,
            checkout_kind,
            run_index,
        );

        if Instant::now() >= next_draw || finished_runners >= runners {
            draw_app(terminal, app)?;
            next_draw += UI_TICK;
            let now = Instant::now();
            if next_draw <= now {
                next_draw = now + UI_TICK;
            }
        }
    }

    join_runner_threads(handles)?;
    if interrupted {
        return Err("Benchmark interrupted.".to_owned());
    }
    if channel_disconnected {
        return Err("Benchmark event channel closed unexpectedly.".to_owned());
    }

    Ok(result)
}

fn run_plain_phase(
    app: &mut AppState,
    run_index: usize,
    run_count: usize,
    checkout_kind: CheckoutKind,
    checkout_label: &str,
    root: &Path,
    run: &RunConfig,
    iterations: usize,
    runners: usize,
    base_seed: u64,
) -> Result<BenchmarkResult, String> {
    app.start_phase(run_index, checkout_label);
    println!(
        "Phase start: run {}/{} | {} | {}",
        run_index + 1,
        run_count,
        checkout_label,
        run.display_args
    );

    let (tx, rx) = mpsc::channel();
    let cancel = Arc::new(AtomicBool::new(false));
    let handles = spawn_runner_threads(
        root.to_path_buf(),
        run.clone(),
        iterations,
        runners,
        base_seed,
        cancel,
        tx,
    );

    let mut result = BenchmarkResult::default();
    let mut finished_runners = 0;
    let mut channel_disconnected = false;
    let mut progress = PlainPhaseProgress::new();
    while finished_runners < runners {
        match rx.recv() {
            Ok(event) => process_ui_event(
                event,
                app,
                &mut result,
                &mut finished_runners,
                checkout_kind,
                run_index,
            ),
            Err(_) => {
                channel_disconnected = true;
                break;
            }
        }
        progress.maybe_print(
            checkout_label,
            run,
            &result,
            finished_runners,
            runners,
            iterations,
        );
    }

    join_runner_threads(handles)?;
    if channel_disconnected {
        return Err("Benchmark event channel closed unexpectedly.".to_owned());
    }

    println!(
        "Phase complete: {} | {} | {} | solved {} ({:.1}%)",
        checkout_label,
        run.display_args,
        result.score_display(),
        result.solved_runs,
        result.solved_percentage()
    );
    Ok(result)
}

fn drain_ui_events(
    rx: &Receiver<UiEvent>,
    app: &mut AppState,
    result: &mut BenchmarkResult,
    finished_runners: &mut usize,
    checkout_kind: CheckoutKind,
    run_index: usize,
) {
    for event in rx.try_iter().take(MAX_UI_EVENTS_PER_FRAME) {
        process_ui_event(
            event,
            app,
            result,
            finished_runners,
            checkout_kind,
            run_index,
        );
    }
}

fn process_ui_event(
    event: UiEvent,
    app: &mut AppState,
    result: &mut BenchmarkResult,
    finished_runners: &mut usize,
    checkout_kind: CheckoutKind,
    run_index: usize,
) {
    match event {
        UiEvent::RunnerStarted {
            runner,
            assigned_iterations,
        } => {
            app.runner_started(runner, assigned_iterations);
        }
        UiEvent::IterationStarted { runner, iteration } => {
            app.iteration_started(runner, iteration);
        }
        UiEvent::Progress { runner, progress } => {
            app.update_runner_progress(runner, progress);
        }
        UiEvent::IterationComplete { runner, score } => {
            app.iteration_complete(runner, &score);
            match score {
                Some(score) => result.record_solved(score),
                None => result.record_no_solution(),
            }
            app.set_result(checkout_kind, run_index, result.clone());
        }
        UiEvent::RunnerFinished { runner } => {
            *finished_runners += 1;
            app.runner_finished(runner);
        }
    }
}

impl PlainPhaseProgress {
    fn new() -> Self {
        Self {
            next_report: Instant::now() + PLAIN_PROGRESS_INTERVAL,
        }
    }

    fn maybe_print(
        &mut self,
        checkout_label: &str,
        run: &RunConfig,
        result: &BenchmarkResult,
        finished_runners: usize,
        runners: usize,
        iterations: usize,
    ) {
        let now = Instant::now();
        if now < self.next_report {
            return;
        }

        let completed = result.solved_runs + result.no_solution_runs;
        println!(
            "Progress: {} | {} | completed {}/{} | solved {} ({:.1}%) | runners {}/{}",
            checkout_label,
            run.display_args,
            completed,
            iterations,
            result.solved_runs,
            result.solved_percentage(),
            finished_runners,
            runners
        );
        self.next_report = now + PLAIN_PROGRESS_INTERVAL;
    }
}

fn join_runner_threads(handles: Vec<JoinHandle<Result<(), String>>>) -> Result<(), String> {
    for handle in handles {
        match handle.join() {
            Ok(Ok(())) => {}
            Ok(Err(err)) => return Err(err),
            Err(_) => return Err("Benchmark runner thread panicked.".to_owned()),
        }
    }
    Ok(())
}

fn spawn_runner_threads(
    root: PathBuf,
    run: RunConfig,
    iterations: usize,
    runners: usize,
    base_seed: u64,
    cancel: Arc<AtomicBool>,
    tx: Sender<UiEvent>,
) -> Vec<JoinHandle<Result<(), String>>> {
    let mut handles = Vec::with_capacity(runners);
    for runner in 0..runners {
        let root = root.clone();
        let run = run.clone();
        let tx = tx.clone();
        let cancel = cancel.clone();
        let assigned = assigned_iterations(iterations, runners, runner);
        handles.push(std::thread::spawn(move || {
            run_runner(root, run, assigned, base_seed, runner, cancel, tx)
        }));
    }
    handles
}

fn run_runner(
    root: PathBuf,
    run: RunConfig,
    iterations: Vec<usize>,
    base_seed: u64,
    runner: usize,
    cancel: Arc<AtomicBool>,
    tx: Sender<UiEvent>,
) -> Result<(), String> {
    send_ui_event(
        &tx,
        UiEvent::RunnerStarted {
            runner,
            assigned_iterations: iterations.len(),
        },
    );

    for iteration in iterations {
        if cancel.load(Ordering::Relaxed) {
            send_ui_event(&tx, UiEvent::RunnerFinished { runner });
            return Ok(());
        }

        send_ui_event(
            &tx,
            UiEvent::IterationStarted {
                runner,
                iteration: iteration + 1,
            },
        );

        let finished_event =
            run_child_iteration(&root, &run, base_seed, iteration, runner, &cancel, &tx)?;
        if cancel.load(Ordering::Relaxed) {
            send_ui_event(&tx, UiEvent::RunnerFinished { runner });
            return Ok(());
        }

        send_ui_event(
            &tx,
            UiEvent::IterationComplete {
                runner,
                score: finished_event,
            },
        );
    }

    send_ui_event(&tx, UiEvent::RunnerFinished { runner });
    Ok(())
}

fn run_child_iteration(
    root: &Path,
    run: &RunConfig,
    base_seed: u64,
    iteration: usize,
    runner: usize,
    cancel: &AtomicBool,
    tx: &Sender<UiEvent>,
) -> Result<Option<Score>, String> {
    let mut command = Command::new(release_binary_path(root));
    command
        .current_dir(root)
        .arg("--progress-stream")
        .args(&run.cli_args)
        .env(
            "WASSIGN_SEED",
            base_seed.wrapping_add(iteration as u64).to_string(),
        )
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    #[cfg(unix)]
    command.process_group(0);

    let mut child = command.spawn().map_err(|err| {
        format!(
            "Could not start wassign for runner {} iteration {}: {err}",
            runner + 1,
            iteration + 1
        )
    })?;

    let stdout = child
        .stdout
        .take()
        .ok_or_else(|| "wassign stdout was not piped.".to_owned())?;
    let stderr = child
        .stderr
        .take()
        .ok_or_else(|| "wassign stderr was not piped.".to_owned())?;

    let stderr_handle = std::thread::spawn(move || read_to_string(stderr));
    let (event_tx, event_rx) = mpsc::channel();
    let stdout_handle = std::thread::spawn(move || read_progress_events(stdout, event_tx));

    let mut finished_event = None;
    let exit_status = loop {
        if cancel.load(Ordering::Relaxed) {
            terminate_child(&mut child);
            break child
                .wait()
                .map_err(|err| format!("Could not wait for canceled wassign: {err}"))?;
        }

        match event_rx.recv_timeout(UI_TICK) {
            Ok(Ok(ProgressStreamEvent::Progress { progress })) => {
                send_ui_event(tx, UiEvent::Progress { runner, progress });
            }
            Ok(Ok(ProgressStreamEvent::Finished { score })) => {
                finished_event = Some(score);
            }
            Ok(Err(err)) => {
                terminate_child(&mut child);
                let _ = child.wait();
                join_child_io_threads(stdout_handle, stderr_handle)?;
                return Err(err);
            }
            Err(RecvTimeoutError::Timeout) => {
                if let Some(status) = child
                    .try_wait()
                    .map_err(|err| format!("Could not poll wassign status: {err}"))?
                {
                    break status;
                }
            }
            Err(RecvTimeoutError::Disconnected) => {
                break child
                    .wait()
                    .map_err(|err| format!("Could not wait for wassign: {err}"))?;
            }
        }
    };

    let stderr_output = join_child_io_threads(stdout_handle, stderr_handle)?;
    if cancel.load(Ordering::Relaxed) {
        return Ok(None);
    }

    if !exit_status.success() {
        return Err(if stderr_output.trim().is_empty() {
            format!(
                "wassign failed for runner {} iteration {}",
                runner + 1,
                iteration + 1
            )
        } else {
            format!(
                "wassign failed for runner {} iteration {}: {}",
                runner + 1,
                iteration + 1,
                stderr_output.trim()
            )
        });
    }

    Ok(finished_event.unwrap_or(None))
}

fn read_progress_events(
    stdout: impl std::io::Read,
    event_tx: Sender<Result<ProgressStreamEvent, String>>,
) -> Result<(), String> {
    let reader = BufReader::new(stdout);
    for line in reader.lines() {
        let line = line.map_err(|err| format!("Could not read wassign output: {err}"))?;
        let event = serde_json::from_str::<ProgressStreamEvent>(&line)
            .map_err(|err| format!("Could not deserialize wassign progress event `{line}`: {err}"));
        if event_tx.send(event).is_err() {
            break;
        }
    }
    Ok(())
}

fn join_child_io_threads(
    stdout_handle: JoinHandle<Result<(), String>>,
    stderr_handle: JoinHandle<Result<String, String>>,
) -> Result<String, String> {
    stdout_handle
        .join()
        .map_err(|_| "wassign stdout collector panicked.".to_owned())??;
    stderr_handle
        .join()
        .map_err(|_| "wassign stderr collector panicked.".to_owned())?
}

fn terminate_child(child: &mut Child) {
    #[cfg(unix)]
    {
        let process_group = format!("-{}", child.id());
        let _ = Command::new("kill")
            .args(["-TERM", process_group.as_str()])
            .status();
        std::thread::sleep(Duration::from_millis(50));
        if child.try_wait().ok().flatten().is_none() {
            let _ = Command::new("kill")
                .args(["-KILL", process_group.as_str()])
                .status();
        }
    }
    #[cfg(not(unix))]
    {
        let _ = child.kill();
    }
}

fn send_ui_event(tx: &Sender<UiEvent>, event: UiEvent) {
    let _ = tx.send(event);
}

fn read_to_string(stream: impl std::io::Read) -> Result<String, String> {
    let mut reader = BufReader::new(stream);
    let mut output = String::new();
    std::io::Read::read_to_string(&mut reader, &mut output)
        .map_err(|err| format!("Could not read child stderr: {err}"))?;
    Ok(output)
}

fn assigned_iterations(iterations: usize, runners: usize, runner: usize) -> Vec<usize> {
    (runner..iterations).step_by(runners.max(1)).collect()
}

fn benchmark_config_path(config_path: Option<PathBuf>) -> PathBuf {
    config_path.unwrap_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("benchmark.toml"))
}

fn read_config(path: &Path) -> Result<BenchmarkConfig, String> {
    let text = fs::read_to_string(path)
        .map_err(|err| format!("Could not read benchmark config {}: {err}", path.display()))?;
    toml::from_str(&text)
        .map_err(|err| format!("Could not parse benchmark config {}: {err}", path.display()))
}

fn prepare_run(run_spec: &str) -> Result<RunConfig, String> {
    Ok(RunConfig {
        display_args: if run_spec.trim().is_empty() {
            "(no args)".to_owned()
        } else {
            run_spec.to_owned()
        },
        cli_args: shell_words(run_spec)?,
    })
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

fn workspace_root() -> Result<PathBuf, String> {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .canonicalize()
        .map_err(|err| format!("Could not determine workspace root: {err}"))
}

fn resolve_compare_commit(value: &str) -> Result<String, String> {
    git_stdout(["rev-parse", value])
}

fn cleanup_stale_benchmark_worktrees(repo_root: &Path) -> Result<(), String> {
    for path in benchmark_worktree_paths(repo_root)? {
        println!("Removing stale benchmark worktree {}...", path.display());
        cleanup_worktree_path(repo_root, &path);
    }
    Ok(())
}

fn benchmark_worktree_paths(repo_root: &Path) -> Result<Vec<PathBuf>, String> {
    let output = Command::new("git")
        .arg("-C")
        .arg(repo_root)
        .args(["worktree", "list", "--porcelain"])
        .output()
        .map_err(|err| format!("Could not list git worktrees: {err}"))?;
    if !output.status.success() {
        return Err(String::from_utf8_lossy(&output.stderr).trim().to_owned());
    }

    let repo_root = repo_root
        .canonicalize()
        .map_err(|err| format!("Could not canonicalize workspace root: {err}"))?;
    let paths = String::from_utf8_lossy(&output.stdout)
        .lines()
        .filter_map(|line| line.strip_prefix("worktree "))
        .map(PathBuf::from)
        .filter(|path| {
            path.file_name()
                .and_then(|name| name.to_str())
                .is_some_and(|name| name.starts_with(WORKTREE_PREFIX))
        })
        .filter(|path| path.canonicalize().map_or(true, |path| path != repo_root))
        .collect();
    Ok(paths)
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

fn create_temporary_worktree(
    repo_root: &Path,
    compare_commit: &str,
    cleaner: WorktreeCleaner,
) -> Result<TemporaryWorktree, String> {
    let path = std::env::temp_dir().join(format!("{WORKTREE_PREFIX}{}", unique_id()));
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
            "git worktree add failed for comparison commit {compare_commit}"
        ));
    }

    cleaner.track(path.clone());
    Ok(TemporaryWorktree { path, cleaner })
}

fn cleanup_worktree_path(repo_root: &Path, path: &Path) {
    let _ = Command::new("git")
        .arg("-C")
        .arg(repo_root)
        .args(["worktree", "remove", "--force"])
        .arg(path)
        .status();
    let _ = fs::remove_dir_all(path);
    let _ = Command::new("git")
        .arg("-C")
        .arg(repo_root)
        .args(["worktree", "prune"])
        .status();
}

impl WorktreeCleaner {
    fn new(repo_root: PathBuf) -> Self {
        Self {
            repo_root,
            paths: Arc::new(Mutex::new(Vec::new())),
        }
    }

    fn track(&self, path: PathBuf) {
        if let Ok(mut paths) = self.paths.lock() {
            paths.push(path);
        }
    }

    fn cleanup_path(&self, path: &Path) {
        cleanup_worktree_path(&self.repo_root, path);
        if let Ok(mut paths) = self.paths.lock() {
            paths.retain(|tracked| tracked != path);
        }
    }

    fn cleanup_all(&self) {
        let paths = self.paths.lock().map_or_else(
            |_| Vec::new(),
            |paths| paths.iter().cloned().collect::<Vec<_>>(),
        );
        for path in paths {
            cleanup_worktree_path(&self.repo_root, &path);
        }
        if let Ok(mut paths) = self.paths.lock() {
            paths.clear();
        }
    }

    fn install_ctrlc_handler(&self) -> Result<(), String> {
        let cleaner = self.clone();
        ctrlc::set_handler(move || {
            cleaner.cleanup_all();
            std::process::exit(130);
        })
        .map_err(|err| format!("Could not install Ctrl+C cleanup handler: {err}"))
    }
}

fn build_release_binary(root: &Path, label: &str) -> Result<(), String> {
    println!("Building release wassign binary for {label}...");
    let status = Command::new("cargo")
        .current_dir(root)
        .args(["build", "--release", "-p", "wassign"])
        .status()
        .map_err(|err| format!("Could not build release wassign binary for {label}: {err}"))?;

    if status.success() {
        println!("Built release wassign binary for {label}.");
        return Ok(());
    }

    Err(format!(
        "Could not build release wassign binary for {label}"
    ))
}

fn release_binary_path(root: &Path) -> PathBuf {
    let binary = if cfg!(windows) {
        "wassign.exe"
    } else {
        "wassign"
    };
    root.join("target").join("release").join(binary)
}

fn ensure_supported_comparison_checkout(root: &Path) -> Result<(), String> {
    let required_paths = [
        root.join("wassign/Cargo.toml"),
        root.join("wassign-core/Cargo.toml"),
    ];
    if required_paths.iter().all(|path| path.exists()) {
        return Ok(());
    }

    Err(
        "comparison commit does not contain the benchmark-compatible workspace layout; \
         only commits with wassign and wassign-core workspace members are supported"
            .to_owned(),
    )
}

fn init_terminal() -> Result<BenchTerminal, String> {
    enable_raw_mode().map_err(|err| format!("Could not enable raw mode: {err}"))?;
    let mut stdout = std::io::stdout();
    execute!(stdout, EnterAlternateScreen)
        .map_err(|err| format!("Could not enter alternate screen: {err}"))?;
    let backend = CrosstermBackend::new(stdout);
    Terminal::new(backend).map_err(|err| format!("Could not initialize terminal: {err}"))
}

fn restore_terminal(terminal: &mut BenchTerminal) -> Result<(), String> {
    disable_raw_mode().map_err(|err| format!("Could not disable raw mode: {err}"))?;
    execute!(terminal.backend_mut(), LeaveAlternateScreen)
        .map_err(|err| format!("Could not leave alternate screen: {err}"))?;
    terminal
        .show_cursor()
        .map_err(|err| format!("Could not show cursor: {err}"))
}

fn handle_input() -> Result<bool, String> {
    if event::poll(Duration::ZERO).map_err(|err| err.to_string())?
        && let CrosstermEvent::Key(key) = event::read().map_err(|err| err.to_string())?
        && (matches!(key.code, KeyCode::Char('q') | KeyCode::Esc)
            || matches!(key.code, KeyCode::Char('c') if key.modifiers.contains(KeyModifiers::CONTROL)))
    {
        return Ok(true);
    }
    Ok(false)
}

fn draw_app(terminal: &mut BenchTerminal, app: &AppState) -> Result<(), String> {
    terminal
        .draw(|frame| render_app(frame, app))
        .map_err(|err| format!("Could not draw benchmark UI: {err}"))?;
    Ok(())
}

fn render_app(frame: &mut Frame<'_>, app: &AppState) {
    let layout = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Length(8), Constraint::Fill(1)])
        .split(frame.area());

    render_result_table(frame, layout[0], app);
    render_runner_panes(frame, layout[1], app);
}

fn render_result_table(frame: &mut Frame<'_>, area: Rect, app: &AppState) {
    let mut widths = Vec::with_capacity(app.run_labels.len() + 1);
    widths.push(Constraint::Length(22));
    widths.extend((0..app.run_labels.len()).map(|_| Constraint::Fill(1)));

    let header = Row::new(
        std::iter::once(Cell::from("metric"))
            .chain(app.run_labels.iter().map(|label| Cell::from(label.clone())))
            .collect::<Vec<_>>(),
    )
    .style(Style::default().add_modifier(Modifier::BOLD));

    let rows = vec![
        summary_row(
            "working score",
            &app.working_results,
            BenchmarkResult::score_display,
        ),
        summary_row(
            "working solved",
            &app.working_results,
            BenchmarkResult::solved_display,
        ),
        summary_row(
            &format!("{} score", app.compare_short_commit),
            &app.comparison_results,
            BenchmarkResult::score_display,
        ),
        summary_row(
            &format!("{} solved", app.compare_short_commit),
            &app.comparison_results,
            BenchmarkResult::solved_display,
        ),
    ];

    let table = Table::new(rows, widths)
        .header(header)
        .block(
            Block::default()
                .title(format!("Result Table | {}", app.phase_status))
                .borders(Borders::ALL),
        )
        .column_spacing(1);
    frame.render_widget(table, area);
}

fn render_runner_panes(frame: &mut Frame<'_>, area: Rect, app: &AppState) {
    let runner_count = app.runner_states.len();
    if runner_count == 0 {
        return;
    }

    let grid = runner_grid(area.height, runner_count);
    let rows = Layout::default()
        .direction(Direction::Vertical)
        .constraints(vec![Constraint::Fill(1); grid.rows])
        .split(area);

    for (row_index, row) in rows.iter().enumerate() {
        let columns = Layout::default()
            .direction(Direction::Horizontal)
            .constraints(vec![Constraint::Fill(1); grid.columns])
            .split(*row);

        for (column_index, chunk) in columns.iter().enumerate() {
            let index = row_index * grid.columns + column_index;
            let Some(state) = app.runner_states.get(index) else {
                continue;
            };
            let lines = runner_dashboard_lines(state);
            let paragraph = Paragraph::new(lines)
                .block(
                    Block::default()
                        .title(state.title.clone())
                        .borders(Borders::ALL),
                )
                .wrap(Wrap { trim: false });
            frame.render_widget(paragraph, *chunk);
        }
    }
}

#[derive(Debug, Clone, Copy)]
struct RunnerGrid {
    rows: usize,
    columns: usize,
}

fn runner_grid(available_height: u16, runner_count: usize) -> RunnerGrid {
    let max_rows = usize::from((available_height / MIN_RUNNER_PANEL_HEIGHT).max(1));
    let rows = runner_count.min(max_rows).max(1);
    let columns = runner_count.div_ceil(rows).max(1);
    RunnerGrid { rows, columns }
}

fn runner_dashboard_lines(state: &RunnerPaneState) -> Vec<Line<'static>> {
    let mut lines = vec![
        Line::raw(format!("status: {}", state.status_display())),
        Line::raw(format!(
            "iterations: {}/{}",
            state.completed_iterations, state.assigned_iterations
        )),
        Line::raw(format!(
            "current: {}",
            state
                .current_iteration
                .map_or_else(|| "n/a".to_owned(), |iteration| iteration.to_string())
        )),
    ];

    if let Some(progress) = &state.latest_progress {
        lines.push(Line::raw(format!(
            "best score: {}",
            if progress.best_score.is_finite() {
                progress.best_score.to_str()
            } else {
                "no solution yet".to_owned()
            }
        )));
        lines.push(Line::raw(format!(
            "sched depth: {:.1}/{:.1}",
            progress.sched_depth, progress.max_sched_depth
        )));
        lines.push(Line::raw(format!(
            "work units: {} it / {} assign / {} lp",
            progress.iterations, progress.assignments, progress.lp
        )));
        lines.push(Line::raw(format!(
            "remaining: {} ms",
            progress.milliseconds_remaining
        )));
    } else {
        lines.push(Line::raw("best score: waiting for progress"));
        lines.push(Line::raw("sched depth: n/a"));
        lines.push(Line::raw("work units: n/a"));
        lines.push(Line::raw("remaining: n/a"));
    }

    lines.push(Line::raw(format!(
        "last result: {}",
        state
            .last_result
            .clone()
            .unwrap_or_else(|| "n/a".to_owned())
    )));
    lines
}

fn summary_row(
    label: &str,
    results: &[Option<BenchmarkResult>],
    formatter: fn(&BenchmarkResult) -> String,
) -> Row<'static> {
    let mut cells = Vec::with_capacity(results.len() + 1);
    cells.push(Cell::from(label.to_owned()));
    cells.extend(results.iter().map(|result| {
        let text = result.as_ref().map_or_else(|| "...".to_owned(), formatter);
        Cell::from(text)
    }));
    Row::new(cells)
}

fn print_final_summary(runs: &[RunConfig], app: &AppState) {
    for (index, run) in runs.iter().enumerate() {
        if let Some(result) = &app.working_results[index] {
            print_summary_block(&format!("{} [working]", run.display_args), result);
        }
        if let Some(result) = &app.comparison_results[index] {
            print_summary_block(
                &format!("{} [{}]", run.display_args, app.compare_short_commit),
                result,
            );
        }
    }
}

fn print_summary_block(display_args: &str, result: &BenchmarkResult) {
    println!("{}", style(display_args).color256(214));
    println!(
        "{}",
        style(format!(
            "({}, {}) solved {} ({:.1}%)",
            format_score_term(result.average_major()),
            format_score_term(result.average_minor()),
            result.solved_runs,
            result.solved_percentage(),
        ))
        .color256(214)
    );
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

impl BenchmarkResult {
    fn record_solved(&mut self, score: Score) {
        if score.major().is_finite() {
            self.major_sum += f64::from(score.major());
            self.major_count += 1;
        }
        self.minor_sum += f64::from(score.minor());
        self.solved_runs += 1;
    }

    fn record_no_solution(&mut self) {
        self.no_solution_runs += 1;
    }

    fn average_major(&self) -> Option<f64> {
        (self.major_count > 0).then(|| self.major_sum / self.major_count as f64)
    }

    fn average_minor(&self) -> Option<f64> {
        (self.solved_runs > 0).then(|| self.minor_sum / self.solved_runs as f64)
    }

    fn solved_percentage(&self) -> f64 {
        let total = self.solved_runs + self.no_solution_runs;
        if total == 0 {
            0.0
        } else {
            (self.solved_runs as f64 / total as f64) * 100.0
        }
    }

    fn score_display(&self) -> String {
        format!(
            "({}, {})",
            format_score_term(self.average_major()),
            format_score_term(self.average_minor()),
        )
    }

    fn solved_display(&self) -> String {
        format!("{} ({:.1}%)", self.solved_runs, self.solved_percentage())
    }
}

impl AppState {
    fn new(run_labels: Vec<String>, compare_short_commit: String, runners: usize) -> Self {
        let runner_states = (0..runners)
            .map(|index| RunnerPaneState::new(format!("Runner {}", index + 1)))
            .collect::<Vec<_>>();

        Self {
            working_results: vec![None; run_labels.len()],
            comparison_results: vec![None; run_labels.len()],
            runner_states,
            phase_status: "Starting benchmark".to_owned(),
            run_labels,
            compare_short_commit,
        }
    }

    fn start_phase(&mut self, run_index: usize, checkout_label: &str) {
        self.phase_status = format!("{} | {}", self.run_labels[run_index], checkout_label);
        for (index, state) in self.runner_states.iter_mut().enumerate() {
            *state = RunnerPaneState::new(format!("Runner {} | {}", index + 1, checkout_label));
        }
    }

    fn runner_started(&mut self, runner: usize, assigned_iterations: usize) {
        let Some(state) = self.runner_states.get_mut(runner) else {
            return;
        };
        state.assigned_iterations = assigned_iterations;
        state.status = RunnerStatus::Waiting;
    }

    fn iteration_started(&mut self, runner: usize, iteration: usize) {
        let Some(state) = self.runner_states.get_mut(runner) else {
            return;
        };
        state.status = RunnerStatus::Running;
        state.current_iteration = Some(iteration);
        state.latest_progress = None;
    }

    fn update_runner_progress(&mut self, runner: usize, progress: ThreadedSolverProgress) {
        let Some(state) = self.runner_states.get_mut(runner) else {
            return;
        };
        state.status = RunnerStatus::Running;
        state.latest_progress = Some(progress);
    }

    fn iteration_complete(&mut self, runner: usize, score: &Option<Score>) {
        let Some(state) = self.runner_states.get_mut(runner) else {
            return;
        };
        state.completed_iterations += 1;
        state.last_result = Some(
            score
                .as_ref()
                .map_or_else(|| "no solution".to_owned(), |score| score.to_str()),
        );
    }

    fn runner_finished(&mut self, runner: usize) {
        let Some(state) = self.runner_states.get_mut(runner) else {
            return;
        };
        state.status = RunnerStatus::Finished;
        state.current_iteration = None;
    }

    fn set_result(&mut self, kind: CheckoutKind, run_index: usize, result: BenchmarkResult) {
        match kind {
            CheckoutKind::Working => self.working_results[run_index] = Some(result),
            CheckoutKind::Comparison => self.comparison_results[run_index] = Some(result),
        }
    }
}

impl RunnerPaneState {
    fn new(title: String) -> Self {
        Self {
            title,
            status: RunnerStatus::Waiting,
            assigned_iterations: 0,
            current_iteration: None,
            completed_iterations: 0,
            latest_progress: None,
            last_result: None,
        }
    }

    fn status_display(&self) -> &'static str {
        match self.status {
            RunnerStatus::Waiting => "waiting",
            RunnerStatus::Running => "running",
            RunnerStatus::Finished => "finished",
        }
    }
}
