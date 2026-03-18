#include "utils.h"

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
