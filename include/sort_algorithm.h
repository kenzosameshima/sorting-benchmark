#ifndef SORT_ALGORITHM_H
#define SORT_ALGORITHM_H

#include <stddef.h>

#include "sorting.h"

typedef void (*SortAlgorithmFunction)(Image arr[],
                                      int n,
                                      Image temp[],
                                      Metrics *metrics);

typedef struct {
  const char *name;
  const char *description;
  SortAlgorithmFunction sort;
  int is_quadratic;
  int is_stable;
  int needs_temp_buffer;
} SortAlgorithm;

const SortAlgorithm *get_sort_algorithms(size_t *count);

#endif
