#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>

typedef struct {
  double mean;
  double median;
  double stddev;
  double min;
  double max;
} TimeStats;

typedef struct {
  double *times;
  uint64_t comparisons;
  uint64_t swaps;
  uint64_t movements;
  int runs;
  int skipped;
} AlgorithmResult;

TimeStats calculate_time_stats(const double times[], int count);
int initialize_algorithm_results(AlgorithmResult results[],
                                 int algorithm_count,
                                 int repetitions);
void reset_algorithm_result(AlgorithmResult *result);
void free_algorithm_results(AlgorithmResult results[], int algorithm_count);

#endif
