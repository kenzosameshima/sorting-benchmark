#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>

#include "generator.h"

typedef struct {
  uint32_t seed;
  int repetitions;
  int max_size;
  int skip_quadratic_after;
  DatasetType dataset_type;
  const int *sizes;
  int size_count;
} BenchmarkConfig;

void run_experiments(const BenchmarkConfig *config);

#endif
