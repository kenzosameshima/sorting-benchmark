#include "sort_algorithm.h"

static void run_bubble(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  bubble_sort(arr, n, metrics);
}

static void run_selection(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  selection_sort(arr, n, metrics);
}

static void run_insertion(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  insertion_sort(arr, n, metrics);
}

static void run_quick(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  quick_sort(arr, n, metrics);
}

static void run_quick_naive(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  quick_sort_naive(arr, n, metrics);
}

static void run_merge(Image arr[], int n, Image temp[], Metrics *metrics) {
  merge_sort(arr, n, temp, metrics);
}

static void run_heap(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  heap_sort(arr, n, metrics);
}

static void run_radix(Image arr[], int n, Image temp[], Metrics *metrics) {
  radix_sort(arr, n, temp, metrics);
}

static void run_shell(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  shell_sort(arr, n, metrics);
}

static void run_qsort_libc(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  qsort_libc(arr, n, metrics);
}

static void run_std_sort(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  std_sort(arr, n, metrics);
}

static void run_std_stable_sort(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  std_stable_sort(arr, n, metrics);
}

const SortAlgorithm *get_sort_algorithms(size_t *count) {
  static const SortAlgorithm algorithms[] = {
    {"bubble", "Quadratic adjacent-swap stable sort", run_bubble, 1, 1, 0},
    {"selection", "Quadratic minimum-selection in-place sort", run_selection, 1, 0, 0},
    {"insertion", "Quadratic stable insertion sort", run_insertion, 1, 1, 0},
    {"quick", "Median-of-three three-way quicksort", run_quick, 0, 0, 0},
    {"quick_naive", "First-element-pivot quicksort, quadratic on sorted/reversed input", run_quick_naive, 1, 0, 0},
    {"merge", "Stable top-down merge sort with external buffer", run_merge, 0, 1, 1},
    {"heap", "In-place binary heap sort", run_heap, 0, 0, 0},
    {"radix", "LSD radix sort (base 256) over the sort key", run_radix, 0, 1, 1},
    {"shell", "Shell sort with Knuth gaps (3k+1)", run_shell, 0, 0, 0},
    {"qsort", "C library qsort baseline (only comparisons are counted)", run_qsort_libc, 0, 0, 0},
    {"std_sort", "C++ std::sort (introsort) baseline (only comparisons are counted)", run_std_sort, 0, 0, 0},
    {"std_stable_sort", "C++ std::stable_sort baseline (only comparisons are counted)", run_std_stable_sort, 0, 1, 0}
  };

  if (count) {
    *count = sizeof(algorithms) / sizeof(algorithms[0]);
  }

  return algorithms;
}
