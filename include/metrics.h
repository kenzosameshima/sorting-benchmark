#ifndef METRICS_H
#define METRICS_H

typedef struct {
  long comparisons;
  long swaps;
  long movements;
} Metrics;

void reset_metrics(Metrics *m);

#endif
