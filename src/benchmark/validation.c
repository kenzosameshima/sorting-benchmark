#include "validation.h"

#include <stdlib.h>

#include "utils.h"

int has_valid_id_permutation(const Image images[], int n) {
  unsigned char *seen;

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
    int id = images[i].id;

    if (id < 0 || id >= n || seen[id] != 0U) {
      free(seen);
      return 0;
    }

    seen[id] = 1U;
  }

  free(seen);
  return 1;
}

int validate_sorted_images(const Image images[], int n) {
  return is_sorted_by_size(images, n) && has_valid_id_permutation(images, n);
}
