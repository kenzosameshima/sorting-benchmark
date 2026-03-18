#include "sorting.h"

static void swap(Image *a, Image *b, Metrics *metrics) {
  Image temp = *a;
  *a = *b;
  *b = temp;
  metrics->swaps++;
  metrics->movements += 3;
}

void bubble_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  for (int i = 0; i + 1 < n; i++) {
    for (int j = 0; j + 1 < n - i; j++) {
      metrics->comparisons++;
      if (arr[j].size > arr[j + 1].size) {
        swap(&arr[j], &arr[j + 1], metrics);
      }
    }
  }
}
