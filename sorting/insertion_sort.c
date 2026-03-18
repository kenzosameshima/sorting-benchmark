#include "sorting.h"

void insertion_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  for (int i = 1; i < n; i++) {
    Image key = arr[i];
    int j = i - 1;
    metrics->movements++;

    while (j >= 0) {
      metrics->comparisons++;
      if (arr[j].size > key.size) {
        arr[j + 1] = arr[j];
        metrics->movements++;
        j--;
      } else {
        break;
      }
    }

    arr[j + 1] = key;
    metrics->movements++;
  }
}
