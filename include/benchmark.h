#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stddef.h>  // for size_t
#include "sort.h"    // or "sort_stats.h" — whichever defines sort_stats

// Define a function pointer type for sorting functions:
// They receive an int array and its size, and sort in-place.
typedef void (*sort_function)(int *, size_t, sort_stats *);

// Measures time (in milliseconds) taken by the sorting function on the given array.
// The input array will be copied internally to avoid modifying the original data.
// Returns elapsed time in milliseconds as double.
double benchmark_sort(sort_function sort_func, const int* arr, size_t size, sort_stats* stats);

#endif // BENCHMARK_H
