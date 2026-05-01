#include "sorting.h"

static void swap(Image *a, Image *b, Metrics *metrics) {
  if (a == b) {
    return;
  }

  Image temp = *a;
  *a = *b;
  *b = temp;
  metrics->swaps++;
  metrics->movements += 3;
}

static int median_of_three(Image arr[], int a, int b, int c, Metrics *metrics) {
  metrics->comparisons++;
  if (arr[a].size > arr[b].size) {
    int temp = a;
    a = b;
    b = temp;
  }

  metrics->comparisons++;
  if (arr[a].size > arr[c].size) {
    int temp = a;
    a = c;
    c = temp;
  }

  metrics->comparisons++;
  if (arr[b].size > arr[c].size) {
    int temp = b;
    b = c;
    c = temp;
  }

  return b;
}

static void partition_3way(Image arr[], int low, int high, int *lt, int *gt, Metrics *metrics) {
  int mid = low + (high - low) / 2;
  int pivot_index = median_of_three(arr, low, mid, high, metrics);

  swap(&arr[low], &arr[pivot_index], metrics);
  Image pivot = arr[low];
  metrics->movements++;

  *lt = low;
  *gt = high;
  int i = low;

  while (i <= *gt) {
    metrics->comparisons++;
    if (arr[i].size < pivot.size) {
      swap(&arr[*lt], &arr[i], metrics);
      (*lt)++;
      i++;
      continue;
    }

    metrics->comparisons++;
    if (arr[i].size > pivot.size) {
      swap(&arr[i], &arr[*gt], metrics);
      (*gt)--;
    } else {
      i++;
    }
  }
}

static void quick_sort_impl(Image arr[], int low, int high, Metrics *metrics) {
  if (low < high) {
    int lt;
    int gt;

    partition_3way(arr, low, high, &lt, &gt, metrics);
    quick_sort_impl(arr, low, lt - 1, metrics);
    quick_sort_impl(arr, gt + 1, high, metrics);
  }
}

void quick_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  quick_sort_impl(arr, 0, n - 1, metrics);
}
