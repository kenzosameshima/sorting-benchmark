#include "common.h"

static int partition_first_pivot(Image arr[], int low, int high, Metrics *metrics) {
  Image pivot = arr[low];
  int i = low + 1;

  for (int j = low + 1; j <= high; j++) {
    metrics->comparisons++;
    if (arr[j].size < pivot.size) {
      if (i != j) {
        swap_images_counted(&arr[i], &arr[j], metrics);
      }
      i++;
    }
  }

  if (i - 1 != low) {
    swap_images_counted(&arr[low], &arr[i - 1], metrics);
  }
  return i - 1;
}

static void quick_sort_naive_impl(Image arr[], int low, int high, Metrics *metrics) {
  while (low < high) {
    int pivot_index = partition_first_pivot(arr, low, high, metrics);

    quick_sort_naive_impl(arr, low, pivot_index - 1, metrics);
    low = pivot_index + 1;
  }
}

void quick_sort_naive(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  quick_sort_naive_impl(arr, 0, n - 1, metrics);
}
