# Sorting Benchmark

Benchmark project for comparing sorting algorithms using execution time and operation
counters with reproducible input generation. The benchmark is written in C, with
a small C++ part for the standard-library baselines; Python is used only for offline
reporting and analysis.

## Features

- Algorithms included: bubble, selection, insertion, quick, quick_naive, merge,
  heap, radix, shell, plus the library
  baselines qsort (C), std_sort and std_stable_sort (C++).
- Multiple input sizes, configurable with `--sizes`.
- Dataset patterns: random, sorted, reversed, nearly_sorted, few_unique,
  organ_pipe, or all.
- `--mode quick|robust` presets: a fast random-only smoke run, or the full
  dataset x size matrix.
- Repeated runs per size with median and standard deviation of elapsed time.
- CSV output with seed, timestamp, compiler, and compilation flags.
- Strict compilation flags with warnings treated as errors.
- Reproducible datasets using an explicit benchmark seed.
- Fair benchmark methodology: one base dataset is generated per repetition and
  copied for every algorithm before sorting.

## Project Structure

- include/: shared headers
- src/: entry point and utilities
- src/sorting/: sorting algorithms
- src/sorting/registry.c: single registry for algorithm metadata and adapters
- src/data/: input data generation
- src/metrics/: metric reset and counters support
- src/benchmark/: benchmark runner, statistics, and validation
- src/output/: CSV writer
- tests/test_sorting.c: sorting, validation and golden-counter tests (C)
- tests/test_report.py: tests for the report tooling (Python)
- scripts/run_suite.py: build + test + benchmark + environment capture
- scripts/build_report.py, scripts/report_template.html: interactive HTML report
- scripts/analyze_results.py: legacy matplotlib plots (see the note below)
- CMakeLists.txt, CMakePresets.json: primary build system (the Makefile still works)
- .github/workflows/ci.yml: CI on Ubuntu and Windows
- results/: generated CSV output of a direct `./benchmark` run
- runs/: one directory per `run_suite.py` run (CSV, `run_info.json`, log; git-ignored)

## Build Requirements

- GCC or Clang with C17 and C++17 support (`gcc`+`g++` or `clang`+`clang++`) (MinGW/MSYS2 UCRT64 on Windows; MSVC is not supported)
- CMake 3.16+ and Ninja (recommended), or make / mingw32-make
- Python 3.8+ for `scripts/run_suite.py` and `scripts/build_report.py` (standard library only)
- clang-format for `make format`
- cppcheck for `make lint`

## Recommended Workflow (CMake + run_suite)

One command that builds with strict warnings, runs the tests, measures, and records
the environment. It behaves the same on Windows, Ubuntu and WSL:

```bash
python scripts/run_suite.py --profile quick --report
```

Profiles: `smoke` (CI), `quick` (sizes 100..10000, 15 reps), `academic`
(sizes 100..100000, 30 reps). Any option can be overridden (`--seed`, `--sizes`,
`--repetitions`, `--dataset`, `--cc clang`, ...). Results go to `runs/<label>/`:

- `results/results.csv`, `results/summary.csv`: the benchmark output
- `run_info.json`: CPU, OS/kernel, WSL flag, compiler (banner and target), CMake
  configuration, power plan/governor, git commit and dirty flag, command line,
  pinned CPU and priority
- `run.log`, and `report.html` when `--report` is given

The script refuses to benchmark a build whose tests fail. It pins the benchmark to
one CPU and raises its priority (`--pin none` / `--no-priority` disable this). On a
hybrid Intel CPU it prefers a performance core (Windows and native Linux); WSL2 does
not expose the topology, so pass `--pin N` explicitly there.

Direct CMake usage (presets `release`, `debug`, `strict`, `sanitize`):

```bash
cmake --preset release && cmake --build --preset release && ctest --preset release
```

The presets use Ninja. Without Ninja on Windows, pass a generator explicitly, for
example `cmake -S . -B build/mingw -G "MinGW Makefiles"` (this generator requires
that `sh.exe` is **not** on `PATH`; with Git Bash/MSYS2 on `PATH` use Ninja or
`-G "MSYS Makefiles"`). `run_suite.py` selects a working generator automatically.

CMake builds with `-O2` and no `-DNDEBUG`, like the Makefile, so results stay
comparable with earlier runs.

## Comparing Windows and Ubuntu

1. Run the same profile with the same seed on each OS (use the same compiler and
   version on both if you want to isolate the OS effect):
   `python scripts/run_suite.py --profile academic --seed 42`
2. Copy the run directories to one machine and build the cross-platform report:
   `python scripts/build_report.py runs/windows-... runs/wsl-... -o report.html`
3. Open `report.html`. It shows the environment of each run, integrity checks,
   interactive curves with error bars, a run-to-run ratio table with bootstrap
   confidence intervals, and per-cell data-quality flags.

The operation counters are deterministic: with the same seed they must be identical
on every OS and compiler. The report checks this and `build_report.py
--fail-on-mismatch` exits with status 3 otherwise (this is what CI uses).

Known limitations of the time measurements (also stated in the report):

- The counters are updated inside the timed region, so times include instrumentation
  overhead that differs per algorithm and per compiler.
- There is no warm-up run and the algorithm order is fixed; the first repetition is
  often slower (flagged as *cold start*).
- Cross-OS time ratios mix OS, compiler and libc effects unless the compiler is the
  same. WSL2 runs inside a VM.

## Continuous Integration

`.github/workflows/ci.yml` runs on every push and pull request:

- `build-test`: Ubuntu (gcc, clang, both with `SB_STRICT`, and gcc with ASan+UBSan)
  and Windows (MSYS2 UCRT64 gcc): build with `-Werror`, then `ctest`
- `benchmark`: `run_suite.py` on Ubuntu and Windows with the same seed
- `compare`: builds the cross-platform report, publishes it as an artifact and in the
  job summary, and **fails if counters differ between the two OSes**

Timings on shared runners are noisy; treat them as indicative and the counter check
as the hard gate. `workflow_dispatch` accepts a `profile` input for longer runs.

## Build and Run

Default build:

- Linux/macOS: make
- Windows (MinGW): mingw32-make

Build benchmark:

- Linux/macOS: make bench
- Windows (MinGW): mingw32-make bench

Run benchmark:

- Linux/macOS: make run
- Windows (MinGW): mingw32-make run

Reduced benchmark:

- Linux/macOS: ./benchmark --seed 42 --repetitions 5 --max-size 1000
- Windows (MinGW): benchmark.exe --seed 42 --repetitions 5 --max-size 1000

Run all dataset patterns over explicit sizes:

- Linux/macOS: ./benchmark --seed 42 --repetitions 5 --sizes 100,1000,10000 --dataset all
- Windows (MinGW): benchmark.exe --seed 42 --repetitions 5 --sizes 100,1000,10000 --dataset all

Quick smoke run (`--mode quick`) or the full dataset x size matrix (`--mode robust`):

- Linux/macOS: ./benchmark --mode quick
- Linux/macOS: ./benchmark --mode robust
- Windows (MinGW): benchmark.exe --mode quick
- Windows (MinGW): benchmark.exe --mode robust

Flags passed after `--mode` override its defaults.

Academic benchmark profile:

- Linux/macOS: ./benchmark --seed 42 --repetitions 30 --skip-quadratic-after 10000
- Windows (MinGW): benchmark.exe --seed 42 --repetitions 30 --skip-quadratic-after 10000

Equivalent Make target:

- Linux/macOS: make academic-bench
- Windows (MinGW): mingw32-make academic-bench

The benchmark executable is named `./benchmark`.

Run unit tests:

- Linux/macOS: make test
- Windows (MinGW): mingw32-make test
- Any OS with CMake: `ctest --test-dir build/cmake-release --output-on-failure`

The C tests include **golden counters**: `comparisons`, `swaps` and `movements` for
every algorithm x dataset at n=100 and n=1000 with seed 42 are pinned in
`tests/test_sorting.c`. They are deterministic and were verified identical on
MinGW gcc 15, Ubuntu gcc 13 and clang 18. A failure means an algorithm, a counter,
the RNG or a generator changed. If that change is intentional, regenerate the table
and review the diff. `tests/test_report.py` covers the report statistics and the
counter cross-check (`python -m unittest discover -s tests -p test_report.py`).

Clean artifacts:

- Linux/macOS: make clean
- Windows (MinGW): mingw32-make clean

Rebuild from scratch:

- Linux/macOS: make rebuild
- Windows (MinGW): mingw32-make rebuild

Debug build:

- Linux/macOS: make DEBUG=1
- Windows (MinGW): mingw32-make DEBUG=1

Sanitizer test build:

- Linux/macOS: make asan-test
- Windows (MinGW): set ASAN_OPTIONS=detect_leaks=0 && mingw32-make SANITIZE=1 test

Sanitizer build and test:

- Linux/macOS: make sanitize
- Windows (MinGW): mingw32-make sanitize

Static analysis:

- Linux/macOS: make lint
- Windows (MinGW): mingw32-make lint

`make lint` requires `cppcheck` to be installed and available in `PATH`.

Format source files:

- Linux/macOS: make format
- Windows (MinGW): mingw32-make format

`make format` requires `clang-format` to be installed and available in `PATH`.

Reproducible benchmark run:

- Linux/macOS: make run ARGS="--seed 12345"
- Windows (MinGW): mingw32-make run ARGS="--seed 12345"

Show make targets:

- Linux/macOS: make help
- Windows (MinGW): mingw32-make help

## Compilation Policy

The Makefile uses strict flags, including:

- -std=c17
- -Wall -Wextra -Wpedantic
- -Wconversion -Wsign-conversion -Wshadow
- -Wformat=2 -Wcast-align -Wmissing-declarations
- -Wstrict-overflow=5 -Wundef
- -Werror

Build artifacts are isolated by configuration under build/release,
build/debug, build/sanitize, and strict variants such as build/release-strict.

## Benchmark Output

The benchmark generates:

- results/results.csv: detailed per-run measurements
- results/summary.csv: aggregated statistics per algorithm and size

Detailed CSV header:

- algorithm,size,rep,seed,dataset_type,timestamp,compiler,flags,status,time,comparisons,swaps,movements

Summary CSV header:

- algorithm,size,seed,dataset_type,timestamp,compiler,flags,repetitions,status,time_mean,time_median,time_stddev,time_min,time_max,average_comparisons,average_swaps,average_movements

Notes:

- When an expensive run is intentionally skipped, status is `SKIPPED` and metric
  fields are written as `NA`.
- Quadratic algorithms are skipped for very large inputs when above the
  configured threshold.
- For a given size and repetition, every algorithm receives a copy of the same
  `base_dataset`. The detailed CSV records `rep`, `seed`, and `dataset_type` to
  make this pairing explicit.

## Metrics

| Metric | Meaning |
| --- | --- |
| comparisons | Logical comparisons between elements |
| swaps | Complete swaps between two `Image` elements |
| movements | Assignments/copies of complete `Image` elements |
| time_mean | Mean elapsed time across repetitions |
| time_median | Median elapsed time across repetitions |
| time_stddev | Standard deviation of elapsed time |

`merge_sort` counts both copies to the auxiliary buffer and writes back to the
original array.

## Analyzing Benchmark Results

Prefer the interactive report (`scripts/build_report.py`, see above): it needs no
dependencies, keeps datasets and sizes separate, and shows confidence intervals.

The matplotlib script below is kept for quick static plots. Note that it averages
`time_mean` across datasets (and, for the dataset plot, across sizes), which mixes
very different behaviours (for example insertion sort on sorted vs reversed input),
so read its plots with care.

Install dependencies (recommended: use a virtual environment).

Linux/WSL (recommended):

```bash
# create and activate a venv in the project (or use a system venv in your WSL home)
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

Alternatively, install Debian-packaged Python dependencies system-wide:

```bash
sudo apt update
sudo apt install python3-pandas python3-matplotlib
```

Run the benchmark:

```bash
./benchmark --seed 42 --repetitions 5 --sizes 1000,10000,100000 --dataset all
```

Generate plots (runs inside the activated venv):

```bash
python scripts/analyze_results.py --input results/summary.csv --output plots
```

Generated plots:

```text
plots/runtime_by_size.png
plots/runtime_by_dataset.png
plots/comparisons_by_size.png
```

Note: Python is used only for offline analysis; benchmark execution remains implemented in C.

## Experimental Environment

Recorded automatically in `runs/<label>/run_info.json` by `scripts/run_suite.py` and
shown in the report: CPU, RAM, operating system and kernel (including WSL2), compiler
banner and target, compilation flags, number of repetitions, RNG seed, pinned CPU,
priority, power plan/governor, and git commit. Runs made by calling `./benchmark`
directly only have the `compiler`, `flags` and `seed` columns of the CSV.

## Reproducibility

Use `--seed` to repeat the same input sequence:

- Linux/macOS: make run ARGS="--seed 12345"
- Windows (MinGW): mingw32-make run ARGS="--seed 12345"

The selected seed is written to `results/results.csv` along with the timestamp,
compiler version, and compilation flags.

## Algorithm Registry

Algorithms are registered once in `src/sorting/registry.c` through
`SortAlgorithm` metadata:

- `name`: stable CSV/test identifier
- `description`: human-readable algorithm summary
- `sort`: adapter with the common benchmark/test signature
- `is_quadratic`: enables skip policy for expensive sizes
- `is_stable`: used by tests that validate stable algorithms
- `needs_temp_buffer`: documents algorithms such as merge sort

Both the benchmark runner and the test suite call `get_sort_algorithms()`.
Adding an algorithm should require one sorting implementation, one adapter entry
in the registry, and no benchmark/test loop changes.

## Dataset Generation

Dataset generation is centralized in `src/data/generator.c`.

- `random`: random sizes in the `0..9999` range, shuffled by id
- `sorted`: ascending sizes
- `reversed`: descending sizes
- `nearly_sorted`: ascending data with about 1% random swaps
- `few_unique`: random values in a small `0..9` range, then shuffled
- `organ_pipe`: ascending then descending (ramps up to the midpoint, then back down)
- `all`: benchmark convenience mode that runs every concrete dataset type

Validation uses sorted-order checks plus O(n) id-permutation validation, so the
benchmark confirms every generated element id appears exactly once without the
old quadratic item-matching pass.

## Algorithms Evaluated

- bubble
- selection
- insertion
- quick
- quick_naive: first-element-pivot quicksort, quadratic on sorted/reversed
  input, kept alongside the hardened `quick` to show what pivot choice buys you
- merge
- heap
- radix: LSD radix sort (base 256) over the sort key, with no comparisons
- shell: Shell sort with Knuth gaps (3k+1), between the quadratic sorts and `n log n`
- qsort: C library `qsort` as a baseline; only comparisons are counted (swaps and
  movements happen inside libc and are reported as 0), and it is not guaranteed stable
- std_sort / std_stable_sort: C++ `std::sort` (introsort) and `std::stable_sort` as
  baselines, in `src/sorting/std_sort.cpp` exposed through `extern "C"`; only
  comparisons are counted. They need a C++17 compiler (`g++`/`clang++`).

## Contributing

1. Follow the style guide in C_STYLE_GUIDE.md.
2. Keep functions focused and modular.
3. Preserve strict-flag compatibility (no warnings).
