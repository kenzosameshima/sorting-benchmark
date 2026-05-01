#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>

typedef struct {
  uint64_t comparisons;
  uint64_t swaps;
  uint64_t movements;
} Metrics;

void reset_metrics(Metrics *m);

#endif
