#include "generator.h"

void rng_seed(Rng *rng, uint32_t seed) {
  if (!rng) {
    return;
  }

  rng->state = seed == 0U ? 0xA341316CU : seed;
}

uint32_t rng_next(Rng *rng) {
  uint32_t x = rng->state;

  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng->state = x;
  return x;
}

int rng_range(Rng *rng, int limit) {
  if (limit <= 0) {
    return 0;
  }

  return (int)(rng_next(rng) % (uint32_t)limit);
}

void generate_random_images(Image images[], int n, Rng *rng) {
  if (n <= 0) {
    return;
  }

  for (int i = 0; i < n; i++) {
    images[i].id = i;
  }

  for (int i = 0; i < n; i++) {
    images[i].size = rng_range(rng, 10000);
  }

  for (int i = n; i > 1; i--) {
    int j = rng_range(rng, i);
    int current = i - 1;
    Image temp = images[current];
    images[current] = images[j];
    images[j] = temp;
  }
}
