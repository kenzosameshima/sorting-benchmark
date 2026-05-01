#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>

#include "sorting.h"

typedef struct {
  uint32_t state;
} Rng;

void rng_seed(Rng *rng, uint32_t seed);
uint32_t rng_next(Rng *rng);
int rng_range(Rng *rng, int limit);
void generate_random_images(Image images[], int n, Rng *rng);

#endif
