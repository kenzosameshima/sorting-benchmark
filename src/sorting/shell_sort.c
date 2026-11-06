#include "sorting.h"

/* Knuth gaps (3k+1): a simple sequence with O(n^1.5) worst case. */
void shell_sort(Image arr[], int n, Metrics *metrics) {
  int gap = 1;

  reset_metrics(metrics);

  while (gap < n / 3) {
    gap = gap * 3 + 1;
  }

  for (; gap > 0; gap /= 3) {
    for (int i = gap; i < n; i++) {
      Image key = arr[i];
      int j = i;
      metrics->movements++;

      while (j >= gap) {
        metrics->comparisons++;
        if (arr[j - gap].size > key.size) {
          arr[j] = arr[j - gap];
          metrics->movements++;
          j -= gap;
        } else {
          break;
        }
      }

      arr[j] = key;
      metrics->movements++;
    }
  }
}
