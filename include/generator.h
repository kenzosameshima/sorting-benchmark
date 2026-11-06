#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>

#include "sorting.h"

typedef enum {
  DATASET_RANDOM,
  DATASET_SORTED,
  DATASET_REVERSED,
  DATASET_NEARLY_SORTED,
  DATASET_FEW_UNIQUE,
  DATASET_ORGAN_PIPE,
  DATASET_SAWTOOTH,
  DATASET_ZIPF,
  DATASET_WIDE_RANGE,
  DATASET_ALL
} DatasetType;

typedef struct {
  uint32_t state;
} Rng;

void rng_seed(Rng *rng, uint32_t seed);
uint32_t rng_next(Rng *rng);
int rng_range(Rng *rng, int min, int max);
const char *dataset_type_name(DatasetType type);
int parse_dataset_type(const char *name, DatasetType *type);
void generate_images(Image images[], int n, Rng *rng, DatasetType type);
void generate_random_images(Image images[], int n, Rng *rng);

#endif
