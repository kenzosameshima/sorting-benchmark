#include "utils.h"

#include <stdlib.h>

void copy_images(Image dest[], const Image src[], int n) {
  for (int i = 0; i < n; i++) {
    dest[i] = src[i];
  }
}

int is_sorted_by_size(const Image images[], int n) {
  for (int i = 1; i < n; i++) {
    if (images[i - 1].size > images[i].size) {
      return 0;
    }
  }

  return 1;
}

int has_same_items(const Image original[], const Image sorted[], int n) {
  unsigned char *seen;

  (void)original;

  if (n < 0) {
    return 0;
  }

  if (n == 0) {
    return 1;
  }

  seen = calloc((size_t)n, sizeof(*seen));
  if (!seen) {
    return 0;
  }

  for (int i = 0; i < n; i++) {
    int id = sorted[i].id;

    if (id < 0 || id >= n || seen[id] != 0U) {
      free(seen);
      return 0;
    }

    seen[id] = 1U;
  }

  free(seen);
  return 1;
}
