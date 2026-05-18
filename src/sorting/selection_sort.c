#include "common.h"

void selection_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  for (int i = 0; i + 1 < n; i++) {
    int min_idx = i;
    for (int j = i + 1; j < n; j++) {
      metrics->comparisons++;
      if (arr[j].size < arr[min_idx].size) {
        min_idx = j;
      }
    }

    if (min_idx != i) {
      swap_images_counted(&arr[i], &arr[min_idx], metrics);
    }
  }
}
