#include "common.h"

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

  if (pivot_index != low) {
    swap_images_counted(&arr[low], &arr[pivot_index], metrics);
  }
  Image pivot = arr[low];
  metrics->movements++;

  *lt = low;
  *gt = high;
  int i = low;

  while (i <= *gt) {
    metrics->comparisons++;
    if (arr[i].size < pivot.size) {
      if (*lt != i) {
        swap_images_counted(&arr[*lt], &arr[i], metrics);
      }
      (*lt)++;
      i++;
      continue;
    }

    metrics->comparisons++;
    if (arr[i].size > pivot.size) {
      if (i != *gt) {
        swap_images_counted(&arr[i], &arr[*gt], metrics);
      }
      (*gt)--;
    } else {
      i++;
    }
  }
}

static void quick_sort_impl(Image arr[], int low, int high, Metrics *metrics) {
  while (low < high) {
    int lt;
    int gt;
    int left_size;
    int right_size;

    partition_3way(arr, low, high, &lt, &gt, metrics);
    left_size = lt - low;
    right_size = high - gt;

    if (left_size < right_size) {
      quick_sort_impl(arr, low, lt - 1, metrics);
      low = gt + 1;
    } else {
      quick_sort_impl(arr, gt + 1, high, metrics);
      high = lt - 1;
    }
  }
}

void quick_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  quick_sort_impl(arr, 0, n - 1, metrics);
}
