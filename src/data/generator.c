#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
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

int rng_range(Rng *rng, int min, int max) {
  uint32_t range;
  uint32_t threshold;
  uint32_t r;

  if (max < min) {
    fprintf(stderr, "Invalid range\n");
    exit(EXIT_FAILURE);
  }

  range = (uint32_t)(max - min + 1);
  if (range == 0U) {
    fprintf(stderr, "Invalid range\n");
    exit(EXIT_FAILURE);
  }

  threshold = (UINT32_MAX / range) * range;

  do {
    r = rng_next(rng);
  } while (r >= threshold);

  return min + (int)(r % range);
}

const char *dataset_type_name(DatasetType type) {
  switch (type) {
    case DATASET_RANDOM:
      return "random";
    case DATASET_SORTED:
      return "sorted";
    case DATASET_REVERSED:
      return "reversed";
    case DATASET_NEARLY_SORTED:
      return "nearly_sorted";
    case DATASET_FEW_UNIQUE:
      return "few_unique";
    case DATASET_ORGAN_PIPE:
      return "organ_pipe";
    case DATASET_SAWTOOTH:
      return "sawtooth";
    case DATASET_ZIPF:
      return "zipf";
    case DATASET_WIDE_RANGE:
      return "wide_range";
    case DATASET_ALL:
      return "all";
    default:
      return "unknown";
  }
}

int parse_dataset_type(const char *name, DatasetType *type) {
  if (!name || !type) {
    return 0;
  }

  if (strcmp(name, "random") == 0) {
    *type = DATASET_RANDOM;
  } else if (strcmp(name, "sorted") == 0) {
    *type = DATASET_SORTED;
  } else if (strcmp(name, "reversed") == 0) {
    *type = DATASET_REVERSED;
  } else if (strcmp(name, "nearly_sorted") == 0) {
    *type = DATASET_NEARLY_SORTED;
  } else if (strcmp(name, "few_unique") == 0) {
    *type = DATASET_FEW_UNIQUE;
  } else if (strcmp(name, "organ_pipe") == 0) {
    *type = DATASET_ORGAN_PIPE;
  } else if (strcmp(name, "sawtooth") == 0) {
    *type = DATASET_SAWTOOTH;
  } else if (strcmp(name, "zipf") == 0) {
    *type = DATASET_ZIPF;
  } else if (strcmp(name, "wide_range") == 0) {
    *type = DATASET_WIDE_RANGE;
  } else if (strcmp(name, "all") == 0) {
    *type = DATASET_ALL;
  } else {
    return 0;
  }

  return 1;
}

static void initialize_ids(Image images[], int n) {
  for (int i = 0; i < n; i++) {
    images[i].id = i;
  }
}

static void shuffle_images(Image images[], int n, Rng *rng) {
  for (int i = n; i > 1; i--) {
    int j = rng_range(rng, 0, i - 1);
    int current = i - 1;
    swap_images(&images[current], &images[j]);
  }
}

static void generate_sorted_images(Image images[], int n) {
  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = i;
  }
}

static void generate_reversed_images(Image images[], int n) {
  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = n - i;
  }
}

static void generate_nearly_sorted_images(Image images[], int n, Rng *rng) {
  int perturbations = n / 100;

  if (perturbations < 1) {
    perturbations = 1;
  }

  generate_sorted_images(images, n);
  for (int i = 0; i < perturbations; i++) {
    int left = rng_range(rng, 0, n - 1);
    int right = rng_range(rng, 0, n - 1);
    swap_images(&images[left], &images[right]);
  }
}

static void generate_few_unique_images(Image images[], int n, Rng *rng) {
  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = rng_range(rng, 0, 9);
  }
  shuffle_images(images, n, rng);
}

static void generate_organ_pipe_images(Image images[], int n) {
  int half = n / 2;

  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = i < half ? i : n - i;
  }
}

#define SAWTOOTH_TEETH 8
#define ZIPF_RANGE 1000

/* Several ascending runs back to back. */
static void generate_sawtooth_images(Image images[], int n) {
  int period = n / SAWTOOTH_TEETH;

  if (period < 1) {
    period = 1;
  }

  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = i % period;
  }
}

/* Heavily skewed duplicates: size = ZIPF_RANGE / rank gives a 1/x tail, so
 * small values repeat a lot while large ones are rare. */
static void generate_zipf_images(Image images[], int n, Rng *rng) {
  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = ZIPF_RANGE / (1 + rng_range(rng, 0, ZIPF_RANGE - 1));
  }
}

/* Keys spread over the whole 32-bit range, negatives included, unlike the
 * narrow 0..9999 range of `random`. Exercises every radix pass. */
static void generate_wide_range_images(Image images[], int n, Rng *rng) {
  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = (int)rng_next(rng);
  }
}

void generate_images(Image images[], int n, Rng *rng, DatasetType type) {
  if (n <= 0) {
    return;
  }

  switch (type) {
    case DATASET_RANDOM:
      generate_random_images(images, n, rng);
      break;
    case DATASET_SORTED:
      generate_sorted_images(images, n);
      break;
    case DATASET_REVERSED:
      generate_reversed_images(images, n);
      break;
    case DATASET_NEARLY_SORTED:
      generate_nearly_sorted_images(images, n, rng);
      break;
    case DATASET_FEW_UNIQUE:
      generate_few_unique_images(images, n, rng);
      break;
    case DATASET_ORGAN_PIPE:
      generate_organ_pipe_images(images, n);
      break;
    case DATASET_SAWTOOTH:
      generate_sawtooth_images(images, n);
      break;
    case DATASET_ZIPF:
      generate_zipf_images(images, n, rng);
      break;
    case DATASET_WIDE_RANGE:
      generate_wide_range_images(images, n, rng);
      break;
    case DATASET_ALL:
      generate_random_images(images, n, rng);
      break;
    default:
      generate_random_images(images, n, rng);
      break;
  }
}

void generate_random_images(Image images[], int n, Rng *rng) {
  if (n <= 0) {
    return;
  }

  initialize_ids(images, n);
  for (int i = 0; i < n; i++) {
    images[i].size = rng_range(rng, 0, 9999);
  }
  shuffle_images(images, n, rng);
}
