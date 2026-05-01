#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tests.h"

static int parse_seed(const char *value, uint32_t *seed) {
  char *end = NULL;
  unsigned long parsed = strtoul(value, &end, 10);

  if (*value == '\0' || *end != '\0' || parsed > UINT32_MAX) {
    return 0;
  }

  *seed = (uint32_t)parsed;
  return 1;
}

int main(int argc, char *argv[]) {
  uint32_t seed = (uint32_t)time(NULL);

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--seed") == 0) {
      if (i + 1 >= argc || !parse_seed(argv[i + 1], &seed)) {
        fprintf(stderr, "Uso: %s [--seed NUMERO]\n", argv[0]);
        return 1;
      }
      i++;
    } else {
      fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
      fprintf(stderr, "Uso: %s [--seed NUMERO]\n", argv[0]);
      return 1;
    }
  }

  printf("Seed: %u\n", seed);
  run_experiments(seed);
  return 0;
}
