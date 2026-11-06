#include <stdint.h>

#include "common.h"

#define RADIX_BASE 256
#define RADIX_PASSES 4

/* Flipping the sign bit maps signed ints to an unsigned order-preserving
 * key, so unsigned byte-wise radix passes sort negative values correctly. */
static uint32_t radix_key(int value) {
  return (uint32_t)value ^ 0x80000000U;
}

void radix_sort(Image arr[], int n, Image temp[], Metrics *metrics) {
  int count[RADIX_BASE];

  reset_metrics(metrics);

  if (n <= 1) {
    return;
  }

  for (int pass = 0; pass < RADIX_PASSES; pass++) {
    int shift = pass * 8;

    for (int i = 0; i < RADIX_BASE; i++) {
      count[i] = 0;
    }

    for (int i = 0; i < n; i++) {
      int digit = (radix_key(arr[i].size) >> shift) & 0xFF;
      count[digit]++;
    }

    for (int i = 1; i < RADIX_BASE; i++) {
      count[i] += count[i - 1];
    }

    for (int i = n; i-- > 0;) {
      int digit = (radix_key(arr[i].size) >> shift) & 0xFF;
      temp[--count[digit]] = arr[i];
      metrics->movements++;
    }

    for (int i = 0; i < n; i++) {
      arr[i] = temp[i];
    }
  }
}
