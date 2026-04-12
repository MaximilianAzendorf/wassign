//! Terminal benchmark dashboard for comparing the current checkout against a
//! compatible comparison commit.

use std::collections::VecDeque;
use std::ffi::OsStr;
use std::fs;
use std::io::{BufRead, BufReader};
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::sync::mpsc::{self, RecvTimeoutError, Sender};
use std::thread::JoinHandle;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

use console::style;
use crossterm::event::{self, Event as CrosstermEvent, KeyCode};
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

const RUNNER_LOG_LIMIT: usize = 200;
const UI_TICK: Duration = Duration::from_millis(100);

#[derive(Debug, serde::Deserialize)]
struct BenchmarkConfig {
    iterations: usize,
    runners: usize,
    runs: Vec<String>,
    compare_commit: Option<String>,
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
    Log {
        runner: usize,
        line: String,
    },
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

struct AppState {
    run_labels: Vec<String>,
    compare_short_commit: String,
    working_results: Vec<Option<BenchmarkResult>>,
    comparison_results: Vec<Option<BenchmarkResult>>,
    runner_titles: Vec<String>,
    runner_logs: Vec<VecDeque<String>>,
    phase_status: String,
}

type BenchTerminal = Terminal<CrosstermBackend<std::io::Stdout>>;

fn main() {
    if let Err(err) = run() {
        eprintln!("{err}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let config_path = benchmark_config_path();
    let config = read_config(&config_path)?;
    let runs = config
        .runs
        .iter()
        .map(|run_spec| prepare_run(run_spec))
        .collect::<Result<Vec<_>, _>>()?;
    let workspace_root = workspace_root()?;
    let compare_commit = resolve_compare_commit(
        config
            .compare_commit
            .as_deref()
            .unwrap_or("newest"),
    )?;
    let compare_short_commit = git_stdout(["rev-parse", "--short", compare_commit.as_str()])?;
    let worktree = create_temporary_worktree(&workspace_root, &compare_commit)?;
    ensure_supported_comparison_checkout(&worktree.path)?;

    let mut terminal = init_terminal()?;
    let mut app = AppState::new(
        runs.iter().map(|run| run.display_args.clone()).collect(),
        compare_short_commit.clone(),
        config.runners,
    );

    let run_result = run_dashboard(
        &mut terminal,
        &mut app,
        &workspace_root,
        &worktree.path,
        &runs,
        config.iterations,
        config.runners,
    );
    restore_terminal(&mut terminal)?;
    run_result?;
    print_final_summary(&runs, &app);
    Ok(())
}

fn run_dashboard(
    terminal: &mut BenchTerminal,
    app: &mut AppState,
    workspace_root: &Path,
    comparison_root: &Path,
    runs: &[RunConfig],
    iterations: usize,
    runners: usize,
) -> Result<(), String> {
    let base_seed = current_seed();
    for (run_index, run) in runs.iter().enumerate() {
        let run_seed = base_seed.wrapping_add((run_index as u64).wrapping_mul(1_000_000));
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

    app.phase_status = "Benchmark complete".to_owned();
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
    let handles = spawn_runner_threads(
        root.to_path_buf(),
        run.clone(),
        iterations,
        runners,
        base_seed,
        tx,
    );

    let mut result = BenchmarkResult::default();
    let mut finished_runners = 0;
    while finished_runners < runners {
        draw_app(terminal, app)?;
        handle_input()?;

        match rx.recv_timeout(UI_TICK) {
            Ok(UiEvent::Progress { runner, progress }) => {
                app.push_runner_log(runner, format_progress_line(&progress));
            }
            Ok(UiEvent::IterationComplete { runner, score }) => {
                let line = score.as_ref().map_or_else(
                    || "no solution".to_owned(),
                    |score| score.to_str(),
                );
                app.push_runner_log(runner, format!("finished {line}"));
                match score {
                    Some(score) => result.record_solved(score),
                    None => result.record_no_solution(),
                }
                app.set_result(checkout_kind, run_index, result.clone());
            }
            Ok(UiEvent::RunnerFinished { runner }) => {
                finished_runners += 1;
                app.push_runner_log(runner, "runner finished".to_owned());
            }
            Ok(UiEvent::Log { runner, line }) => app.push_runner_log(runner, line),
            Err(RecvTimeoutError::Timeout) => {}
            Err(RecvTimeoutError::Disconnected) => {
                return Err("Benchmark event channel closed unexpectedly.".to_owned());
            }
        }
    }

    for handle in handles {
        match handle.join() {
            Ok(Ok(())) => {}
            Ok(Err(err)) => return Err(err),
            Err(_) => return Err("Benchmark runner thread panicked.".to_owned()),
        }
    }

    Ok(result)
}

fn spawn_runner_threads(
    root: PathBuf,
    run: RunConfig,
    iterations: usize,
    runners: usize,
    base_seed: u64,
    tx: Sender<UiEvent>,
) -> Vec<JoinHandle<Result<(), String>>> {
    let mut handles = Vec::with_capacity(runners);
    for runner in 0..runners {
        let root = root.clone();
        let run = run.clone();
        let tx = tx.clone();
        let assigned = assigned_iterations(iterations, runners, runner);
        handles.push(std::thread::spawn(move || {
            run_runner(root, run, assigned, base_seed, runner, tx)
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
    tx: Sender<UiEvent>,
) -> Result<(), String> {
    send_ui_event(
        &tx,
        UiEvent::Log {
            runner,
            line: format!(
                "starting {} iteration(s) for {}",
                iterations.len(),
                run.display_args
            ),
        },
    );

    for iteration in iterations {
        send_ui_event(
            &tx,
            UiEvent::Log {
                runner,
                line: format!("iteration {} starting", iteration + 1),
            },
        );

        let mut command = Command::new("cargo");
        command
            .current_dir(&root)
            .args(["run", "--quiet", "--release", "-p", "wassign", "--"])
            .arg("--progress-stream")
            .args(&run.cli_args)
            .env("WASSIGN_SEED", base_seed.wrapping_add(iteration as u64).to_string())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped());
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
        let reader = BufReader::new(stdout);
        let mut finished_event = None;
        for line in reader.lines() {
            let line = line.map_err(|err| format!("Could not read wassign output: {err}"))?;
            let event = serde_json::from_str::<ProgressStreamEvent>(&line).map_err(|err| {
                format!("Could not deserialize wassign progress event `{line}`: {err}")
            })?;
            match event {
                ProgressStreamEvent::Progress { progress } => {
                    send_ui_event(&tx, UiEvent::Progress { runner, progress });
                }
                ProgressStreamEvent::Finished { score } => {
                    finished_event = Some(score);
                }
            }
        }

        let stderr_output = stderr_handle
            .join()
            .map_err(|_| "wassign stderr collector panicked.".to_owned())??;
        let status = child
            .wait()
            .map_err(|err| format!("Could not wait for wassign: {err}"))?;
        if !status.success() {
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

        send_ui_event(
            &tx,
            UiEvent::IterationComplete {
                runner,
                score: finished_event.unwrap_or(None),
            },
        );
    }

    send_ui_event(&tx, UiEvent::RunnerFinished { runner });
    Ok(())
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

fn benchmark_config_path() -> PathBuf {
    std::env::args_os()
        .nth(1)
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("benchmark.toml"))
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
    if value == "newest" {
        git_stdout(["rev-parse", "HEAD"])
    } else {
        git_stdout(["rev-parse", value])
    }
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
) -> Result<TemporaryWorktree, String> {
    let path = std::env::temp_dir().join(format!("wassign-benchmark-worktree-{}", unique_id()));
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

    Ok(TemporaryWorktree {
        repo_root: repo_root.to_path_buf(),
        path,
    })
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

fn handle_input() -> Result<(), String> {
    if event::poll(Duration::from_millis(1)).map_err(|err| err.to_string())?
        && let CrosstermEvent::Key(key) = event::read().map_err(|err| err.to_string())?
        && matches!(key.code, KeyCode::Char('q') | KeyCode::Esc)
    {
        return Err("Benchmark interrupted.".to_owned());
    }
    Ok(())
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
        summary_row("working score", &app.working_results, BenchmarkResult::score_display),
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
    let chunks = Layout::default()
        .direction(Direction::Horizontal)
        .constraints(vec![Constraint::Fill(1); app.runner_logs.len()])
        .split(area);

    for (index, chunk) in chunks.iter().enumerate() {
        let lines = app.runner_logs[index]
            .iter()
            .cloned()
            .map(Line::raw)
            .collect::<Vec<_>>();
        let paragraph = Paragraph::new(lines)
            .block(
                Block::default()
                    .title(app.runner_titles[index].clone())
                    .borders(Borders::ALL),
            )
            .wrap(Wrap { trim: false });
        frame.render_widget(paragraph, *chunk);
    }
}

fn summary_row(
    label: &str,
    results: &[Option<BenchmarkResult>],
    formatter: fn(&BenchmarkResult) -> String,
) -> Row<'static> {
    let mut cells = Vec::with_capacity(results.len() + 1);
    cells.push(Cell::from(label.to_owned()));
    cells.extend(results.iter().map(|result| {
        let text = result
            .as_ref()
            .map_or_else(|| "...".to_owned(), formatter);
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

fn format_progress_line(progress: &ThreadedSolverProgress) -> String {
    let counters = format!("{}/{}/{}", progress.iterations, progress.assignments, progress.lp);
    let depth = format!("d{:.1}/{:.1}", progress.sched_depth, progress.max_sched_depth);
    if progress.best_score.is_finite() {
        format!("{} {}", progress.best_score.to_str(), counters)
    } else if progress.lp == 0 && progress.iterations == 0 && progress.assignments == 0 {
        format!("no solution yet {depth}")
    } else {
        format!("no solution yet {depth} {counters}")
    }
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
        let runner_logs = (0..runners)
            .map(|_| VecDeque::with_capacity(RUNNER_LOG_LIMIT))
            .collect::<Vec<_>>();
        let runner_titles = (0..runners)
            .map(|index| format!("Runner {}", index + 1))
            .collect::<Vec<_>>();

        Self {
            working_results: vec![None; run_labels.len()],
            comparison_results: vec![None; run_labels.len()],
            runner_titles,
            runner_logs,
            phase_status: "Starting benchmark".to_owned(),
            run_labels,
            compare_short_commit,
        }
    }

    fn start_phase(&mut self, run_index: usize, checkout_label: &str) {
        self.phase_status = format!("{} | {}", self.run_labels[run_index], checkout_label);
        for (index, log) in self.runner_logs.iter_mut().enumerate() {
            log.clear();
            self.runner_titles[index] = format!("Runner {} | {}", index + 1, checkout_label);
        }
    }

    fn push_runner_log(&mut self, runner: usize, line: String) {
        let Some(log) = self.runner_logs.get_mut(runner) else {
            return;
        };
        if log.back() == Some(&line) {
            return;
        }
        if log.len() >= RUNNER_LOG_LIMIT {
            log.pop_front();
        }
        log.push_back(line);
    }

    fn set_result(&mut self, kind: CheckoutKind, run_index: usize, result: BenchmarkResult) {
        match kind {
            CheckoutKind::Working => self.working_results[run_index] = Some(result),
            CheckoutKind::Comparison => self.comparison_results[run_index] = Some(result),
        }
    }
}
