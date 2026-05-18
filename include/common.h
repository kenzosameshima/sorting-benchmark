#ifndef COMMON_H
#define COMMON_H

#include "sorting.h"

static inline void swap_images(Image *a, Image *b) {
  Image temp = *a;
  *a = *b;
  *b = temp;
}

static inline void swap_images_counted(Image *a, Image *b, Metrics *metrics) {
  Image temp = *a;
  *a = *b;
  *b = temp;

  metrics->swaps++;
  metrics->movements += 3;
}

#endif
