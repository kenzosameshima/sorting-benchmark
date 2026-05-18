#include "common.h"

void bubble_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  for (int i = 0; i + 1 < n; i++) {
    for (int j = 0; j + 1 < n - i; j++) {
      metrics->comparisons++;
      if (arr[j].size > arr[j + 1].size) {
        swap_images_counted(&arr[j], &arr[j + 1], metrics);
      }
    }
  }
}
