# Sorting Benchmark (C)

Benchmark project for comparing sorting algorithms using execution time and operation
counters with reproducible input generation.

## Features

- Algorithms included: bubble, selection, insertion, quick, merge, heap.
- Multiple input sizes, configurable with `--sizes`.
- Dataset patterns: random, sorted, reversed, nearly_sorted, few_unique, or all.
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
- tests/: sorting test suite
- results/: generated CSV output

## Build Requirements

- GCC with C17 support
- make or mingw32-make
- clang-format for `make format`
- cppcheck for `make lint`

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

- CPU:
- RAM:
- Operating System:
- Compiler:
- Compilation Flags:
- Number of Repetitions:
- RNG Seed:

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
- `all`: benchmark convenience mode that runs every concrete dataset type

Validation uses sorted-order checks plus O(n) id-permutation validation, so the
benchmark confirms every generated element id appears exactly once without the
old quadratic item-matching pass.

## Algorithms Evaluated

- bubble
- selection
- insertion
- quick
- merge
- heap

## Contributing

1. Follow the style guide in C_STYLE_GUIDE.md.
2. Keep functions focused and modular.
3. Preserve strict-flag compatibility (no warnings).
