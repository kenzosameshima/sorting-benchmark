#include "statistics.h"

#include <math.h>
#include <stdlib.h>

static int compare_doubles(const void *left, const void *right) {
  double a = *(const double *)left;
  double b = *(const double *)right;

  return (a > b) - (a < b);
}

TimeStats calculate_time_stats(const double times[], int count) {
  TimeStats stats = {0.0, 0.0, 0.0, 0.0, 0.0};
  double sum = 0.0;
  double *sorted;

  if (count <= 0) {
    return stats;
  }

  sorted = malloc((size_t)count * sizeof(double));
  if (!sorted) {
    return stats;
  }

  stats.min = times[0];
  stats.max = times[0];
  for (int i = 0; i < count; i++) {
    sorted[i] = times[i];
    sum += times[i];
    if (times[i] < stats.min) {
      stats.min = times[i];
    }
    if (times[i] > stats.max) {
      stats.max = times[i];
    }
  }

  qsort(sorted, (size_t)count, sizeof(double), compare_doubles);
  if (count % 2 == 0) {
    stats.median = (sorted[count / 2 - 1] + sorted[count / 2]) / 2.0;
  } else {
    stats.median = sorted[count / 2];
  }

  stats.mean = sum / (double)count;
  double variance = 0.0;
  for (int i = 0; i < count; i++) {
    double delta = times[i] - stats.mean;
    variance += delta * delta;
  }
  stats.stddev = sqrt(variance / (double)count);

  free(sorted);
  return stats;
}

void reset_algorithm_result(AlgorithmResult *result) {
  result->times = NULL;
  result->comparisons = 0;
  result->swaps = 0;
  result->movements = 0;
  result->runs = 0;
  result->skipped = 0;
}

int initialize_algorithm_results(AlgorithmResult results[],
                                 int algorithm_count,
                                 int repetitions) {
  for (int i = 0; i < algorithm_count; i++) {
    reset_algorithm_result(&results[i]);
    results[i].times = malloc((size_t)repetitions * sizeof(double));
    if (!results[i].times) {
      free_algorithm_results(results, i);
      return 0;
    }
  }

  return 1;
}

void free_algorithm_results(AlgorithmResult results[], int algorithm_count) {
  for (int i = 0; i < algorithm_count; i++) {
    free(results[i].times);
    results[i].times = NULL;
  }
}
