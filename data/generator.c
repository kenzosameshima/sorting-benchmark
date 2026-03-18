#include <stdlib.h>

#include "generator.h"

void generate_random_images(Image images[], int n) {
  if (n <= 0) {
    return;
  }

  for (int i = 0; i < n; i++) {
    images[i].id = i;
  }

  for (int i = 0; i < n; i++) {
    images[i].size = rand() % 10000;
  }

  for (int i = n; i > 1; i--) {
    int j = rand() % i;
    int current = i - 1;
    Image temp = images[current];
    images[current] = images[j];
    images[j] = temp;
  }
}
