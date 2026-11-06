// C++ standard library baselines. Only comparisons are observable through the
// comparator: swaps and movements happen inside the library and stay 0.
#include <algorithm>

extern "C" {
#include "sorting.h"
}

namespace {

template <typename Sorter>
void run_std(Image arr[], int n, Metrics *metrics, Sorter sorter) {
  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  sorter(arr, arr + n, [metrics](const Image &a, const Image &b) {
    metrics->comparisons++;
    return a.size < b.size;
  });
}

}  // namespace

extern "C" void std_sort(Image arr[], int n, Metrics *metrics) {
  run_std(arr, n, metrics, [](Image *first, Image *last, auto cmp) { std::sort(first, last, cmp); });
}

extern "C" void std_stable_sort(Image arr[], int n, Metrics *metrics) {
  run_std(arr, n, metrics,
          [](Image *first, Image *last, auto cmp) { std::stable_sort(first, last, cmp); });
}
