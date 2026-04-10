# Building wassign

## Linux

To build wassign under Linux, you need the following prerequisites:

* A recent Rust toolchain with `cargo` and `rustc`

After making sure all prerequisites are met you can build wassign:

1. `git clone https://github.com/MaximilianAzendorf/wassign`
2. `cd wassign`
3. `cargo build --release`

The resulting executable is written to `target/release/wassign`.

To run the test suite:

1. `cd wassign`
2. `cargo test`

## Windows

Building on Windows is currently untested, but the project uses the standard Rust/Cargo toolchain there as well.

## Building the documentation

You can build this documentation using [pandoc](https://pandoc.org/) and the respective makefile. Some figures require a LaTeX distribution, inkscape and ghostscript.

If you want a containerized build, use the Docker image based on `pandoc/latex:3.9.0.2-ubuntu`:

1. `cd doc`
2. `make docker-build`

The resulting files are written into `doc/build/` on the host via a bind mount.
