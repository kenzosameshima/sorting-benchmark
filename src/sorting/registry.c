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

static void run_merge(Image arr[], int n, Image temp[], Metrics *metrics) {
  merge_sort(arr, n, temp, metrics);
}

static void run_heap(Image arr[], int n, Image temp[], Metrics *metrics) {
  (void)temp;
  heap_sort(arr, n, metrics);
}

const SortAlgorithm *get_sort_algorithms(size_t *count) {
  static const SortAlgorithm algorithms[] = {
    {"bubble", "Quadratic adjacent-swap stable sort", run_bubble, 1, 1, 0},
    {"selection", "Quadratic minimum-selection in-place sort", run_selection, 1, 0, 0},
    {"insertion", "Quadratic stable insertion sort", run_insertion, 1, 1, 0},
    {"quick", "Median-of-three three-way quicksort", run_quick, 0, 0, 0},
    {"merge", "Stable top-down merge sort with external buffer", run_merge, 0, 1, 1},
    {"heap", "In-place binary heap sort", run_heap, 0, 0, 0}
  };

  if (count) {
    *count = sizeof(algorithms) / sizeof(algorithms[0]);
  }

  return algorithms;
}
