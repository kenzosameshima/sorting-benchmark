# Sorting Benchmark (C)

Benchmark project for comparing sorting algorithms using execution time and operation
counters (comparisons and movements).

## Features

- Algorithms included: bubble, selection, insertion, quick, merge, heap.
- Multiple input sizes: 1000, 10000, 100000.
- Repeated runs per size to compute averages.
- CSV output for later analysis.
- Strict compilation flags with warnings treated as errors.
- Reproducible datasets using an explicit benchmark seed.

## Project Structure

- src/: entry point and utilities
- sorting/: sorting algorithms
- data/: input data generation
- metrics/: metric reset and counters support
- experiments/: benchmark orchestration
- include/: shared headers
- results/: generated CSV output

## Build Requirements

- GCC with C17 support
- make or mingw32-make

## Build and Run

Default build:

- Linux/macOS: make
- Windows (MinGW): mingw32-make

Run benchmark:

- Linux/macOS: make run
- Windows (MinGW): mingw32-make run

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

Reproducible benchmark run:

- Linux/macOS: make run ARGS="--seed 12345"
- Windows (MinGW): benchmark.exe --seed 12345

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

## Benchmark Output

The benchmark generates:

- results/results.csv

CSV header:

- algorithm,size,seed,repetitions,time_median,time_stddev,time_min,time_max,average_comparisons,average_swaps,average_movements

Notes:

- When an expensive run is intentionally skipped, repetitions is 0 and metric
  fields are written as NA.
- Quadratic algorithms are skipped for very large inputs when above the
  configured threshold.

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
