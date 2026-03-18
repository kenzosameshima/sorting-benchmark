#include "sorting.h"

static void swap(Image *a, Image *b, Metrics *metrics) {
  Image temp = *a;
  *a = *b;
  *b = temp;
  metrics->swaps++;
  metrics->movements += 3;
}

static int partition(Image arr[], int low, int high, Metrics *metrics) {
  Image pivot = arr[high];
  metrics->movements++;

  int i = low - 1;
  for (int j = low; j < high; j++) {
    metrics->comparisons++;
    if (arr[j].size <= pivot.size) {
      i++;
      swap(&arr[i], &arr[j], metrics);
    }
  }

  swap(&arr[i + 1], &arr[high], metrics);
  return i + 1;
}

static void quick_sort_recursive(Image arr[], int low, int high, Metrics *metrics) {
  if (low < high) {
    int pi = partition(arr, low, high, metrics);
    quick_sort_recursive(arr, low, pi - 1, metrics);
    quick_sort_recursive(arr, pi + 1, high, metrics);
  }
}

void quick_sort(Image arr[], int low, int high, Metrics *metrics) {
  reset_metrics(metrics);
  quick_sort_recursive(arr, low, high, metrics);
}
