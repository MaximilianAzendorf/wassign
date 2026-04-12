use std::io::{self, Write};
use std::sync::{Arc, Mutex, OnceLock};
use std::thread::sleep;
use std::time::Duration;

use console::style;
use env_logger::{Env, WriteStyle};
use indicatif::{MultiProgress, ProgressBar, ProgressDrawTarget, ProgressStyle};
use log::Level;

use crate::{
    Options, Solution, ThreadedSolverProgress, ThreadedSolverResult, ThreadedSolverRunning,
};

/// Global status output helper used by the CLI.
///
/// # Panics
///
/// Panics if the global status mutex is poisoned.
pub fn enable_output(_options: &Options) {
    initialize_logger();

    let multi_progress = Arc::new(MultiProgress::with_draw_target(
        ProgressDrawTarget::stderr_with_hz(10),
    ));
    let mut state = state().lock().expect("status mutex poisoned");
    state.output = true;
    state.multi_progress = Some(multi_progress);
    state.active_bar = None;
}

/// Emits a trace-level message when trace logging is enabled.
pub fn trace(text: &str) {
    emit(Level::Trace, text);
}

/// Emits a debug-level message when debug logging is enabled.
pub fn debug(text: &str) {
    emit(Level::Debug, text);
}

/// Prints an informational message.
pub fn info(text: &str) {
    emit(Level::Info, text);
}

/// Prints a highlighted informational message.
pub fn info_important(text: &str) {
    emit(Level::Info, text);
}

/// Prints a warning message.
pub fn warning(text: &str) {
    emit(Level::Warn, text);
}

/// Prints an error message.
pub fn error(text: &str) {
    emit(Level::Error, text);
}

/// Tracks a running solver with a progress bar and returns its final solution.
///
/// # Errors
///
/// Returns an error if joining the worker threads fails or a worker panics.
pub fn track_solver(mut solver: ThreadedSolverRunning) -> crate::Result<ThreadedSolverResult> {
    let timeout_ms = u64::try_from(solver.timeout_seconds().max(0)).unwrap_or(0) * 1_000;
    let solving = solver_progress_bar(timeout_ms.max(1));
    solving.set_message(format_solver_message(&solver.progress()));

    while solver.is_running() {
        let progress = solver.progress();
        let remaining_ms = u64::try_from(progress.milliseconds_remaining.max(0)).unwrap_or(0);
        let elapsed_ms = timeout_ms.saturating_sub(remaining_ms);
        solving.set_position(elapsed_ms.min(timeout_ms.max(1)));
        solving.set_message(format_solver_message(&progress));
        sleep(Duration::from_millis(100));
    }

    let final_progress = solver.progress();
    let result = match solver.wait() {
        Ok(result) => result,
        Err(err) => {
            solving.finish_and_clear();
            return Err(err);
        }
    };
    let remaining_ms = u64::try_from(final_progress.milliseconds_remaining.max(0)).unwrap_or(0);
    let elapsed_ms = timeout_ms.saturating_sub(remaining_ms);
    solving.set_position(elapsed_ms.min(timeout_ms.max(1)));
    let summary = if result.solution == Solution::Invalid {
        "No solution found.".to_owned()
    } else {
        format!("Solved {}", format_solver_summary(&final_progress))
    };
    solving.finish_and_clear();
    info_important(&summary);

    Ok(result)
}

/// Tracks a running solver and forwards textual progress updates to a callback.
///
/// This variant is intended for non-CLI integrations such as benchmarks or
/// terminal dashboards that want to display solver progress in a custom UI
/// instead of using the global progress bar.
///
/// # Errors
///
/// Returns an error if joining the worker threads fails or a worker panics.
pub fn track_solver_with_callback<F>(
    mut solver: ThreadedSolverRunning,
    mut callback: F,
) -> crate::Result<ThreadedSolverResult>
where
    F: FnMut(String),
{
    let initial_message = format_solver_message(&solver.progress());
    callback(initial_message.clone());
    let mut last_message = Some(initial_message);

    while solver.is_running() {
        let progress = solver.progress();
        let message = format_solver_message(&progress);
        if last_message.as_ref() != Some(&message) {
            callback(message.clone());
            last_message = Some(message);
        }
        sleep(Duration::from_millis(100));
    }

    let final_progress = solver.progress();
    let result = solver.wait()?;
    let summary = if result.solution == Solution::Invalid {
        "No solution found.".to_owned()
    } else {
        format!("Solved {}", format_solver_summary(&final_progress))
    };
    callback(summary);

    Ok(result)
}

/// Tracks a running solver and forwards progress snapshots to a callback.
///
/// # Errors
///
/// Returns an error if joining the worker threads fails or a worker panics.
pub fn track_solver_with_progress_callback<F>(
    mut solver: ThreadedSolverRunning,
    mut callback: F,
) -> crate::Result<ThreadedSolverResult>
where
    F: FnMut(ThreadedSolverProgress),
{
    let initial_progress = solver.progress();
    callback(initial_progress.clone());
    let mut last_progress = initial_progress;

    while solver.is_running() {
        let progress = solver.progress();
        if progress != last_progress {
            callback(progress.clone());
            last_progress = progress;
        }
        sleep(Duration::from_millis(100));
    }

    let final_progress = solver.progress();
    if final_progress != last_progress {
        callback(final_progress);
    }

    solver.wait()
}

#[derive(Default)]
struct StatusState {
    output: bool,
    multi_progress: Option<Arc<MultiProgress>>,
    active_bar: Option<ProgressBar>,
}

#[derive(Default)]
struct IndicatifLogWriter {
    buffer: Vec<u8>,
}

pub(crate) struct StatusProgress {
    bar: ProgressBar,
    previous_bar: Option<ProgressBar>,
    restores_previous_bar: bool,
}

impl StatusProgress {
    pub(crate) fn inc(&self, delta: u64) {
        self.bar.inc(delta);
    }

    pub(crate) fn set_message(&self, message: impl Into<String>) {
        self.bar.set_message(message.into());
    }

    pub(crate) fn set_position(&self, position: u64) {
        self.bar.set_position(position);
    }

    pub(crate) fn finish_and_clear(&self) {
        self.bar.finish_and_clear();
    }
}

impl Drop for StatusProgress {
    fn drop(&mut self) {
        if !self.restores_previous_bar {
            return;
        }

        let mut state = state().lock().expect("status mutex poisoned");
        state.active_bar.clone_from(&self.previous_bar);
    }
}

impl Write for IndicatifLogWriter {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.buffer.extend_from_slice(buf);
        while let Some(newline_pos) = self.buffer.iter().position(|&byte| byte == b'\n') {
            let line = self.buffer.drain(..=newline_pos).collect::<Vec<_>>();
            let line = String::from_utf8_lossy(&line[..line.len().saturating_sub(1)]).into_owned();
            print_log_line(&line)?;
        }

        Ok(buf.len())
    }

    fn flush(&mut self) -> io::Result<()> {
        if self.buffer.is_empty() {
            return Ok(());
        }

        let line = String::from_utf8_lossy(&self.buffer).into_owned();
        self.buffer.clear();
        print_log_line(&line)
    }
}

pub(crate) fn progress_bar(prefix: &str, length: u64) -> StatusProgress {
    create_progress_bar(
        prefix,
        ProgressBar::new(length.max(1)).with_style(progress_bar_style()),
    )
}

pub(crate) fn solver_progress_bar(length: u64) -> StatusProgress {
    create_progress_bar(
        "",
        ProgressBar::new(length.max(1)).with_style(solver_progress_bar_style()),
    )
}

pub(crate) fn hidden_progress() -> StatusProgress {
    StatusProgress {
        bar: ProgressBar::hidden(),
        previous_bar: None,
        restores_previous_bar: false,
    }
}

fn create_progress_bar(prefix: &str, bar: ProgressBar) -> StatusProgress {
    let mut state = state().lock().expect("status mutex poisoned");
    if !state.output || !log::log_enabled!(Level::Info) {
        return StatusProgress {
            bar: ProgressBar::hidden(),
            previous_bar: None,
            restores_previous_bar: false,
        };
    }

    let Some(multi_progress) = state.multi_progress.as_ref() else {
        return StatusProgress {
            bar: ProgressBar::hidden(),
            previous_bar: None,
            restores_previous_bar: false,
        };
    };

    let bar = multi_progress.add(bar);
    bar.set_prefix(prefix.to_owned());
    let previous_bar = state.active_bar.replace(bar.clone());

    StatusProgress {
        bar,
        previous_bar,
        restores_previous_bar: true,
    }
}

fn emit(level: Level, text: &str) {
    if !state().lock().expect("status mutex poisoned").output {
        return;
    }

    match level {
        Level::Error => log::error!("{text}"),
        Level::Warn => log::warn!("{text}"),
        Level::Info => log::info!("{text}"),
        Level::Debug => log::debug!("{text}"),
        Level::Trace => log::trace!("{text}"),
    }
}

fn format_solver_message(progress: &crate::threaded_solver::ThreadedSolverProgress) -> String {
    let counters = style(format!(
        "{}/{}/{}",
        progress.iterations, progress.assignments, progress.lp
    ))
    .dim()
    .to_string();
    let depth = style(format!(
        "d{:.1}/{:.1}",
        progress.sched_depth, progress.max_sched_depth
    ))
    .dim()
    .to_string();

    if progress.best_score.is_finite() {
        let score = style(progress.best_score.to_str()).yellow().to_string();
        format!("{score} {counters}")
    } else if progress.lp == 0 && progress.iterations == 0 && progress.assignments == 0 {
        format!("{} {depth}", style("no solution yet").dim())
    } else {
        format!("{} {depth} {counters}", style("no solution yet").dim())
    }
}

fn format_solver_summary(progress: &crate::threaded_solver::ThreadedSolverProgress) -> String {
    if progress.best_score.is_finite() {
        format!(
            "{}; i/a/l {}/{}/{}; depth {:.1}/{:.1}",
            progress.best_score.to_str(),
            progress.iterations,
            progress.assignments,
            progress.lp,
            progress.sched_depth,
            progress.max_sched_depth
        )
    } else {
        "no score".to_owned()
    }
}

fn print_log_line(line: &str) -> io::Result<()> {
    let active_bar = state()
        .lock()
        .expect("status mutex poisoned")
        .active_bar
        .clone();
    if let Some(bar) = active_bar {
        bar.println(line);
        return Ok(());
    }

    let mut stderr = io::stderr().lock();
    writeln!(stderr, "{line}")
}

fn progress_bar_style() -> ProgressStyle {
    ProgressStyle::with_template("{prefix:.bold} [{wide_bar:.cyan/blue}] {percent:>3}% {msg} ")
        .expect("progress bar template should be valid")
        .progress_chars("=>-")
}

fn solver_progress_bar_style() -> ProgressStyle {
    ProgressStyle::with_template("{elapsed_precise} [{wide_bar:.cyan/blue}] {msg} ")
        .expect("progress bar template should be valid")
        .progress_chars("=>-")
}

fn state() -> &'static Mutex<StatusState> {
    static STATE: OnceLock<Mutex<StatusState>> = OnceLock::new();
    STATE.get_or_init(|| Mutex::new(StatusState::default()))
}

fn initialize_logger() {
    static LOGGER_INIT: OnceLock<()> = OnceLock::new();
    if LOGGER_INIT.get().is_some() {
        return;
    }

    let default_filter = "info";
    let mut builder =
        env_logger::Builder::from_env(Env::default().default_filter_or(default_filter));
    builder.target(env_logger::Target::Pipe(Box::new(
        IndicatifLogWriter::default(),
    )));
    if std::env::var_os("RUST_LOG_STYLE").is_none() {
        builder.write_style(WriteStyle::Always);
    }
    builder.format(|buf, record| {
        let level_style = buf.default_level_style(record.level());
        writeln!(
            buf,
            "[{level_style}{}{level_style:#}] {}",
            record.level(),
            record.args()
        )
    });
    let _ = builder.try_init();
    let _ = LOGGER_INIT.set(());
}
