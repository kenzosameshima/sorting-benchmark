#include <stdlib.h>

#include "sorting.h"

/* qsort has no user-context argument, so the counter is file-scope.
 * Not thread-safe; the benchmark is single-threaded. */
static Metrics *active_metrics;

static int compare_by_size(const void *a, const void *b) {
  const Image *x = a;
  const Image *y = b;

  active_metrics->comparisons++;
  return (x->size > y->size) - (x->size < y->size);
}

/* Library baseline. Only comparisons are observable: swaps and movements
 * happen inside libc and stay 0. */
void qsort_libc(Image arr[], int n, Metrics *metrics) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  active_metrics = metrics;
  qsort(arr, (size_t)n, sizeof(Image), compare_by_size);
}
