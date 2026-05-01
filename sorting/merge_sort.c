#include <stdlib.h>

#include "sorting.h"

static void merge(Image arr[], int left, int mid, int right, Image arr_copy[], Metrics *metrics) {
  int i = left;
  int j = mid + 1;
  int k = left;

  while (i <= mid && j <= right) {
    metrics->comparisons++;
    if (arr[i].size <= arr[j].size) {
      arr_copy[k++] = arr[i++];
    } else {
      arr_copy[k++] = arr[j++];
    }
    metrics->movements++;
  }

  while (i <= mid) {
    arr_copy[k++] = arr[i++];
    metrics->movements++;
  }

  while (j <= right) {
    arr_copy[k++] = arr[j++];
    metrics->movements++;
  }

  for (i = left; i <= right; i++) {
    arr[i] = arr_copy[i];
    metrics->movements++;
  }
}

static void merge_sort_impl(Image arr[], int left, int right, Image arr_copy[], Metrics *metrics) {
  if (left < right) {
    int mid = left + (right - left) / 2;
    merge_sort_impl(arr, left, mid, arr_copy, metrics);
    merge_sort_impl(arr, mid + 1, right, arr_copy, metrics);
    merge(arr, left, mid, right, arr_copy, metrics);
  }
}

void merge_sort(Image arr[], int n, Metrics *metrics) {
  Image *arr_copy;

  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  arr_copy = malloc((size_t)n * sizeof(Image));
  if (!arr_copy) {
    return;
  }

  merge_sort_impl(arr, 0, n - 1, arr_copy, metrics);
  free(arr_copy);
}
