#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "benchmark.h"

#define MAX_CUSTOM_SIZES 32

static int parse_uint32_arg(const char *value, uint32_t *out) {
  char *end = NULL;
  unsigned long parsed = strtoul(value, &end, 10);

  if (*value == '\0' || *end != '\0' || parsed > UINT32_MAX) {
    return 0;
  }

  *out = (uint32_t)parsed;
  return 1;
}

static int parse_int_arg(const char *value, int *out) {
  char *end = NULL;
  long parsed = strtol(value, &end, 10);

  if (*value == '\0' || *end != '\0' || parsed < 0 || parsed > INT_MAX) {
    return 0;
  }

  *out = (int)parsed;
  return 1;
}

static void print_usage(const char *program) {
  fprintf(stderr,
          "Uso: %s [--seed NUMERO] [--repetitions N] [--max-size N] "
          "[--skip-quadratic-after N] [--dataset TIPO] [--sizes A,B,C]\n"
          "Datasets: random, sorted, reversed, nearly_sorted, few_unique, all\n",
          program);
}

static int parse_sizes_arg(const char *value, int sizes[], int *size_count) {
  const char *current = value;
  int count = 0;

  if (!value || *value == '\0') {
    return 0;
  }

  while (*current != '\0') {
    char *end = NULL;
    long parsed = strtol(current, &end, 10);

    if (current == end || parsed <= 0 || parsed > INT_MAX ||
        count >= MAX_CUSTOM_SIZES) {
      return 0;
    }

    sizes[count] = (int)parsed;
    count++;

    if (*end == '\0') {
      break;
    }
    if (*end != ',') {
      return 0;
    }
    current = end + 1;
  }

  *size_count = count;
  return count > 0;
}

int main(int argc, char *argv[]) {
  int custom_sizes[MAX_CUSTOM_SIZES];
  int custom_size_count = 0;
  BenchmarkConfig config = {
    (uint32_t)time(NULL),
    0,
    0,
    10000,
    DATASET_RANDOM,
    NULL,
    0
  };

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--seed") == 0) {
      if (i + 1 >= argc || !parse_uint32_arg(argv[i + 1], &config.seed)) {
        print_usage(argv[0]);
        return 1;
      }
      i++;
    } else if (strcmp(argv[i], "--repetitions") == 0) {
      if (i + 1 >= argc || !parse_int_arg(argv[i + 1], &config.repetitions) ||
          config.repetitions <= 0) {
        print_usage(argv[0]);
        return 1;
      }
      i++;
    } else if (strcmp(argv[i], "--max-size") == 0) {
      if (i + 1 >= argc || !parse_int_arg(argv[i + 1], &config.max_size) ||
          config.max_size <= 0) {
        print_usage(argv[0]);
        return 1;
      }
      i++;
    } else if (strcmp(argv[i], "--skip-quadratic-after") == 0) {
      if (i + 1 >= argc ||
          !parse_int_arg(argv[i + 1], &config.skip_quadratic_after)) {
        print_usage(argv[0]);
        return 1;
      }
      i++;
    } else if (strcmp(argv[i], "--dataset") == 0) {
      if (i + 1 >= argc ||
          !parse_dataset_type(argv[i + 1], &config.dataset_type)) {
        print_usage(argv[0]);
        return 1;
      }
      i++;
    } else if (strcmp(argv[i], "--sizes") == 0) {
      if (i + 1 >= argc ||
          !parse_sizes_arg(argv[i + 1], custom_sizes, &custom_size_count)) {
        print_usage(argv[0]);
        return 1;
      }
      config.sizes = custom_sizes;
      config.size_count = custom_size_count;
      i++;
    } else {
      fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  printf("Seed: %u\n", config.seed);
  run_experiments(&config);
  return 0;
}
