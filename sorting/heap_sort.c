#include "sorting.h"

static void swap(Image *a, Image *b, Metrics *metrics) {
  Image temp = *a;
  *a = *b;
  *b = temp;
  metrics->swaps++;
  metrics->movements += 3;
}

static void heapify(Image arr[], int n, int i, Metrics *metrics) {
  int largest = i;
  int l = 2 * i + 1;
  int r = 2 * i + 2;

  if (l < n) {
    metrics->comparisons++;
    if (arr[l].size > arr[largest].size) {
      largest = l;
    }
  }

  if (r < n) {
    metrics->comparisons++;
    if (arr[r].size > arr[largest].size) {
      largest = r;
    }
  }

  if (largest != i) {
    swap(&arr[i], &arr[largest], metrics);
    heapify(arr, n, largest, metrics);
  }
}

void heap_sort(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  for (int i = n / 2; i > 0; i--) {
    heapify(arr, n, i - 1, metrics);
  }

  for (int i = n; i > 0; i--) {
    int last = i - 1;
    swap(&arr[0], &arr[last], metrics);
    heapify(arr, last, 0, metrics);
  }
}
