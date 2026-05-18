#include "sorting.h"

static void merge(Image arr[], int left, int mid, int right, Image temp[], Metrics *metrics) {
  int i = left;
  int j = mid + 1;
  int k = left;

  while (i <= mid && j <= right) {
    metrics->comparisons++;
    if (arr[i].size <= arr[j].size) {
      temp[k++] = arr[i++];
    } else {
      temp[k++] = arr[j++];
    }
    metrics->movements++;
  }

  while (i <= mid) {
    temp[k++] = arr[i++];
    metrics->movements++;
  }

  while (j <= right) {
    temp[k++] = arr[j++];
    metrics->movements++;
  }

  for (i = left; i <= right; i++) {
    arr[i] = temp[i];
    metrics->movements++;
  }
}

static void merge_sort_impl(Image arr[], int left, int right, Image temp[], Metrics *metrics) {
  if (left < right) {
    int mid = left + (right - left) / 2;
    merge_sort_impl(arr, left, mid, temp, metrics);
    merge_sort_impl(arr, mid + 1, right, temp, metrics);
    merge(arr, left, mid, right, temp, metrics);
  }
}

void merge_sort(Image arr[], int n, Image temp[], Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  if (!temp) {
    return;
  }

  merge_sort_impl(arr, 0, n - 1, temp, metrics);
}
