#include "metrics.h"

void reset_metrics(Metrics *m) {
  if (!m) {
    return;
  }

  m->comparisons = 0;
  m->swaps = 0;
  m->movements = 0;
}
