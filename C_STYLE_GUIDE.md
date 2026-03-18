# C Style Guide for Algorithm Analysis Project

This guide is based on the Google C++ Style Guide adapted for C, along with
best practices from the Linux Kernel.

## 1. Naming

- Functions: `snake_case` (for example, `bubble_sort`, `generate_random_images`)
- Variables: `snake_case` (for example, `metrics`, `n`, `original`)
- Constants: `ALL_CAPS` (for example, `QUADRATIC_LIMIT`, `MAX_SIZE`)
- Types (`struct`): `UpperCamelCase` (for example, `Image`, `Metrics`)
- Macros: `ALL_CAPS` (for example, `#define BUFFER_SIZE 1024`)

## 2. Structure and Modularity

- Keep each sorting algorithm in a separate `.c` and `.h` file.
- Keep utility functions (copy array, check sorting) in `utils.c`.
- Keep metrics handling in `metrics.c`.
- Keep experiments in `experiments.c`.
- Suggested directories:

```text
sorting_analysis/
  src/
  data/
  metrics/
  sorting/
  experiments/
  include/
```

## 3. Headers

- Always use header guards:

```c
#ifndef SORTING_H
#define SORTING_H
// Declarations
#endif // SORTING_H
```

- `#pragma once` is acceptable if preferred.

## 4. Comments

Document functions, parameters, and return values:

```c
/**
 * @brief Sorts an array of images using bubble sort.
 * @param arr Array of images.
 * @param n Number of elements.
 * @param metrics Structure for counting comparisons, movements, and swaps.
 */
void bubble_sort(Image arr[], int n, Metrics *metrics);
```

Comment critical sections, such as counting comparisons and movements.

## 5. Formatting

- Indentation: 2 or 4 spaces, consistently.
- Braces style:

```c
if (condition) {
    // code
} else {
    // code
}
```

- Prefer lines up to 80-100 characters.

## 6. Safety and Best Practices

Always check return values of critical functions:

```c
if (!ptr) {
    fprintf(stderr, "Allocation error\n");
    return;
}
```

Reset metrics before each execution:

```c
reset_metrics(&metrics);
```

Avoid unnecessary macros; prefer `const int` or `enum` where possible.

## 7. Functions

Keep functions short and focused:

- `swap()`: only swaps values.
- `copy_images()`: only copies values.
- Wrappers (`quick_sort_wrapper`, `merge_sort_wrapper`): handle extra
  allocation or limits.

Avoid deep recursion unless necessary (Quick Sort, Merge Sort).

## 8. Experiment Structure

- Separate data generation, algorithm execution, and metric recording.
- Use wrappers for algorithms that require auxiliary allocation.
- Generate CSV with:

```text
algorithm,size,repetitions,average_time,average_comparisons,average_movements
```

## 9. Compilation

Use strict flags:

```bash
gcc -std=c11 -O2 -Iinclude -Wall -Wextra -Wpedantic \
    -Wconversion -Wsign-conversion -Wshadow -Wformat=2 \
    -Wcast-align -Wmissing-declarations -Wstrict-overflow=5 -Wundef -Werror
```

Ensure warnings are treated as errors (`-Werror`).

## Objective

Following this guide ensures:

- Readable and consistent code.
- Easier maintenance and debugging.
- Reliable metric measurement.
- A structure ready for future expansion (new algorithms or sorting criteria).
