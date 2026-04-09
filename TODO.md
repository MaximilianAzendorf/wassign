# Rust/C++ parity checklist

Legend: `[x]` already present in this repository; `[ ]` missing or stale; `[?]` verify against the C++ behavior before closing.

## Documentation
- [x] `README.md` matches the intended project overview.
- [x] `doc/src/*` is present at the repository root.
- [x] Update the mirrored manual for the Rust DSL: replace `var` examples with `let`, remove the `.name =` preprocessing workaround, and remove `-v`/`--verbosity` from the documented CLI.
- [x] Rework the manual examples around `read_csv`, `slice(1, end)`, and CSV iteration so they match the actual Rust API surface, or add the missing API if parity is intended.
- [x] Keep `realistic120.wassign` and `realistic300.wassign` in sync with the final DSL and use them as parser/smoke fixtures.

## CLI And Runtime Output
- [x] `--help`, `--version`, `-i/--input`, `-o/--output`, `-a/--any`, `-p/--pref-exp`, `-t/--timeout`, `--cs-timeout`, `--no-cs`, `--no-cs-simp`, `-j/--threads`, `-n/--max-neighbors`, and `-g/--greedy` are implemented in `src/options.rs`.
- [x] stdin fallback and concatenation of multiple input files are implemented in `src/main.rs`.
- [x] `-v/--verbosity` is intentionally removed in the Rust port; do not reintroduce it, but make sure all docs and tests stop referring to it.
- [x] Verify help/version banner text, error wording, and output destination behavior against the C++ binary wherever the behavior is still supposed to match.

## DSL And Parser Surface
- [x] Core object constructors exist: `slot`, `choice`, `chooser`, `constraint`, and `+` registration.
- [x] Choice tags already supported in Rust: `min`, `max`, `bounds`, `parts`, `optional`, `optional_if`.
- [x] Native Rhai property access is wired for `.name`, `.choices`, `.size`, `.slot`, and `.choosers`.
- [x] Basic relations already supported in Rust: `==`, `!=`, `<`, `<=`, `>`, `>=`, `contains`, `contains_not`.
- [x] Part expansion for multi-part choices is implemented in `src/input/input_builder.rs`.
- [x] Remove `src/input/input_reader.rs` preprocessing so the DSL uses native Rhai syntax directly; no textual `var` -> `let` rewrite and no `.name =` rewrite.
- [x] Add the missing C++ DSL helpers to Rust: `set_arguments`, `range`, `slice`, `end`, `readFile`, `read_csv`, numeric-string conversions, chooser preference string overloads, and part-aware `choice.slot(part)` / `choice.choosers(part)`.
- [x] Match the meaningful C++ overload behavior for `choice(...)` and `chooser(...)`: `choice` supports 0-4 tagged arguments, `chooser` accepts numeric and string preferences, and unsupported chooser tags remain rejected.
- [x] Decide what to do with `subsetOf` / `supersetOf`: leave them unsupported because the legacy builder never consumed them either.
- [x] Verify duplicate-name handling and fuzzy matching semantics against the legacy Chaiscript reader on ambiguous scripts.

## Solver And Architecture
- [x] The two-stage scheduling/assignment solver pipeline is ported.
- [x] Critical-set analysis, greedy mode, `any` mode, timeouts, and threaded solving are present.
- [x] Generated-part naming, continuation handling, and CSV prefix stripping are implemented.
- [x] Replace the most harmful C++-style shared-state pattern with owned solver-local RNG state; the Rhai input bridge still uses interior mutability at the script boundary.
- [x] Reduce global/singleton-style state in `Rng` by deriving deterministic per-worker streams instead of sharing one mutexed engine. `Status` remains a process-global CLI service.
- [x] Check that solver progress, cancellation, and timeout behavior match the C++ implementation on long-running or multi-threaded inputs.

## Output Behavior
- [x] Scheduling and assignment CSV formatting matches the current C++ layout, including quoted headers and the `not scheduled` sentinel.
- [x] Generated choice prefixes are stripped before output.
- [x] Add regression tests for exact stdout/stderr behavior when no output prefix is given, when `--output` is set, and when a solution is not found.
- [x] Confirm the new logging and progress presentation is a deliberate Rust-specific overhaul, not an accidental behavioral drift.

## Tests
- [x] Input parsing, scheduling solver, assignment solver, and threaded solver all have Rust integration tests.
- [x] `util` and `union_find` parity is covered by Rust unit tests, so the raw test count is still in line with C++.
- [x] The C++ `AssignmentSolverTest` large-case check is intentionally disabled; there is no parity target there unless that test is revived.
- [x] Strengthen `tests/input_test.rs` to assert the same semantics as the C++ `InputTest.cpp`: exact slot, choice, and chooser names, preference normalization, generated-part continuation, and the full constraint set.
- [x] Add parser regression tests for the DSL features missing from the current Rust bindings, especially `read_csv`, `slice(end)`, `range`, `choice.slot(part)`, and `chooser`/`choice` relationship helpers.
- [x] Add CLI-level tests for option parsing and help/version output, especially around the removed `-v` flag and invalid argument handling.
- [x] Add output-format regression tests that compare exact CSV text and file naming behavior, not just normalized strings.
- [x] Add smoke tests that run `realistic120.wassign` and `realistic300.wassign` through the Rust parser/solver once the final DSL surface is settled.
- [?] Re-run the Rust and C++ suites after each parity change so the checklist stays grounded in actual behavior rather than file counts alone. The Rust suite is rerunnable here; the checked-in C++ build tree does not currently expose runnable CTest entries in this workspace.
