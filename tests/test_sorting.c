#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csv_writer.h"
#include "generator.h"
#include "sort_algorithm.h"
#include "sorting.h"
#include "statistics.h"
#include "utils.h"
#include "validation.h"

typedef struct {
  const char *name;
  const int *values;
  int count;
} TestCase;

static void fill_images(Image images[], const int values[], int count) {
  for (int i = 0; i < count; i++) {
    images[i].id = i;
    images[i].size = values[i];
  }
}

static int is_stable_for_equal_sizes(const Image images[], int count) {
  for (int i = 1; i < count; i++) {
    if (images[i - 1].size == images[i].size &&
        images[i - 1].id > images[i].id) {
      return 0;
    }
  }

  return 1;
}

static int run_case(const SortAlgorithm *sorter, const TestCase *test_case) {
  Metrics metrics;
  Image *images = NULL;
  Image *original = NULL;
  Image *temp = NULL;
  int ok;

  if (test_case->count > 0) {
    images = malloc((size_t)test_case->count * sizeof(Image));
    original = malloc((size_t)test_case->count * sizeof(Image));
    temp = malloc((size_t)test_case->count * sizeof(Image));
    if (!images || !original || !temp) {
      fprintf(stderr, "Falha ao alocar caso de teste.\n");
      free(images);
      free(original);
      free(temp);
      return 0;
    }

    fill_images(images, test_case->values, test_case->count);
    copy_images(original, images, test_case->count);
  }

  sorter->sort(images, test_case->count, temp, &metrics);
  ok = is_sorted_by_size(images, test_case->count) &&
       has_same_items(original, images, test_case->count);

  if (sorter->is_stable && ok) {
    ok = is_stable_for_equal_sizes(images, test_case->count);
  }

  if (!ok) {
    fprintf(stderr, "Falha: %s em '%s'.\n", sorter->name, test_case->name);
  }

  free(images);
  free(original);
  free(temp);
  return ok;
}

static int run_random_case(const SortAlgorithm *sorter) {
  Metrics metrics;
  Rng rng;
  const int count = 128;
  Image images[128];
  Image original[128];
  Image temp[128];
  int ok;

  rng_seed(&rng, 12345U);
  generate_random_images(images, count, &rng);
  copy_images(original, images, count);

  sorter->sort(images, count, temp, &metrics);
  ok = is_sorted_by_size(images, count) && has_same_items(original, images, count);

  if (!ok) {
    fprintf(stderr, "Falha: %s em 'random'.\n", sorter->name);
  }

  return ok;
}

static int test_validation_rejects_duplicate_ids(void) {
  Image images[] = {
    {0, 1},
    {1, 2},
    {1, 3}
  };

  return !has_valid_id_permutation(images, (int)(sizeof(images) / sizeof(images[0])));
}

static int test_dataset_generation(void) {
  static const DatasetType datasets[] = {
    DATASET_RANDOM,
    DATASET_SORTED,
    DATASET_REVERSED,
    DATASET_NEARLY_SORTED,
    DATASET_FEW_UNIQUE
  };
  Rng rng;
  Image images[128];

  for (size_t i = 0; i < sizeof(datasets) / sizeof(datasets[0]); i++) {
    rng_seed(&rng, 9876U);
    generate_images(images, (int)(sizeof(images) / sizeof(images[0])), &rng, datasets[i]);
    if (!has_valid_id_permutation(images, (int)(sizeof(images) / sizeof(images[0])))) {
      fprintf(stderr, "Falha: dataset invalido '%s'.\n", dataset_type_name(datasets[i]));
      return 0;
    }
  }

  return 1;
}

static int test_dataset_parse(void) {
  DatasetType type;

  return parse_dataset_type("random", &type) && type == DATASET_RANDOM &&
         parse_dataset_type("sorted", &type) && type == DATASET_SORTED &&
         parse_dataset_type("reversed", &type) && type == DATASET_REVERSED &&
         parse_dataset_type("nearly_sorted", &type) && type == DATASET_NEARLY_SORTED &&
         parse_dataset_type("few_unique", &type) && type == DATASET_FEW_UNIQUE &&
         parse_dataset_type("all", &type) && type == DATASET_ALL &&
         !parse_dataset_type("missing", &type);
}

static int almost_equal(double left, double right) {
  double diff = left > right ? left - right : right - left;

  return diff < 0.000000001;
}

static int test_statistics_module(void) {
  const double times[] = {4.0, 1.0, 3.0, 2.0};
  TimeStats stats = calculate_time_stats(times, (int)(sizeof(times) / sizeof(times[0])));

  return almost_equal(stats.mean, 2.5) &&
         almost_equal(stats.median, 2.5) &&
         almost_equal(stats.min, 1.0) &&
         almost_equal(stats.max, 4.0) &&
         stats.stddev > 0.0;
}

static int test_registry_module(void) {
  size_t count = 0U;
  const SortAlgorithm *algorithms = get_sort_algorithms(&count);

  return algorithms != NULL &&
         count >= 6U &&
         algorithms[0].name != NULL &&
         algorithms[0].description != NULL &&
         algorithms[0].sort != NULL;
}

static int test_csv_writer_module(void) {
  ResultFiles files;

  files.runs = tmpfile();
  files.summary = tmpfile();
  if (!files.runs || !files.summary) {
    close_result_files(&files);
    return 0;
  }

  write_result_headers(&files);
  close_result_files(&files);
  return 1;
}


/*
 * Golden counters: comparisons/swaps/movements are deterministic functions of
 * (algorithm, dataset, size, seed). They were captured with seed 42 and are
 * identical on Windows (MinGW gcc 15), Ubuntu (gcc 13) and clang, so any
 * mismatch means an algorithm, counter, or RNG/generator change.
 * Regenerate deliberately (and review the diff) when one of those changes.
 */
#define GOLDEN_SEED 42U

typedef struct {
  const char *algorithm;
  DatasetType dataset;
  int size;
  uint64_t comparisons;
  uint64_t swaps;
  uint64_t movements;
} GoldenCounters;

static const GoldenCounters golden_counters[] = {
  {"bubble", DATASET_RANDOM, 100, 4950U, 2321U, 6963U},
  {"selection", DATASET_RANDOM, 100, 4950U, 94U, 282U},
  {"insertion", DATASET_RANDOM, 100, 2418U, 0U, 2519U},
  {"quick", DATASET_RANDOM, 100, 1118U, 528U, 1642U},
  {"merge", DATASET_RANDOM, 100, 539U, 0U, 1344U},
  {"heap", DATASET_RANDOM, 100, 1018U, 581U, 1743U},
  {"bubble", DATASET_SORTED, 100, 4950U, 0U, 0U},
  {"selection", DATASET_SORTED, 100, 4950U, 0U, 0U},
  {"insertion", DATASET_SORTED, 100, 99U, 0U, 198U},
  {"quick", DATASET_SORTED, 100, 1619U, 738U, 2270U},
  {"merge", DATASET_SORTED, 100, 356U, 0U, 1344U},
  {"heap", DATASET_SORTED, 100, 1081U, 640U, 1920U},
  {"bubble", DATASET_REVERSED, 100, 4950U, 4950U, 14850U},
  {"selection", DATASET_REVERSED, 100, 4950U, 50U, 150U},
  {"insertion", DATASET_REVERSED, 100, 4950U, 0U, 5148U},
  {"quick", DATASET_REVERSED, 100, 1348U, 624U, 1931U},
  {"merge", DATASET_REVERSED, 100, 316U, 0U, 1344U},
  {"heap", DATASET_REVERSED, 100, 944U, 516U, 1548U},
  {"bubble", DATASET_NEARLY_SORTED, 100, 4950U, 31U, 93U},
  {"selection", DATASET_NEARLY_SORTED, 100, 4950U, 1U, 3U},
  {"insertion", DATASET_NEARLY_SORTED, 100, 130U, 0U, 229U},
  {"quick", DATASET_NEARLY_SORTED, 100, 1624U, 739U, 2274U},
  {"merge", DATASET_NEARLY_SORTED, 100, 371U, 0U, 1344U},
  {"heap", DATASET_NEARLY_SORTED, 100, 1084U, 639U, 1917U},
  {"bubble", DATASET_FEW_UNIQUE, 100, 4950U, 2128U, 6384U},
  {"selection", DATASET_FEW_UNIQUE, 100, 4950U, 90U, 270U},
  {"insertion", DATASET_FEW_UNIQUE, 100, 2226U, 0U, 2326U},
  {"quick", DATASET_FEW_UNIQUE, 100, 605U, 251U, 763U},
  {"merge", DATASET_FEW_UNIQUE, 100, 537U, 0U, 1344U},
  {"heap", DATASET_FEW_UNIQUE, 100, 999U, 544U, 1632U},
  {"bubble", DATASET_RANDOM, 1000, 499500U, 247908U, 743724U},
  {"selection", DATASET_RANDOM, 1000, 499500U, 992U, 2976U},
  {"insertion", DATASET_RANDOM, 1000, 248897U, 0U, 249906U},
  {"quick", DATASET_RANDOM, 1000, 17245U, 9362U, 28633U},
  {"merge", DATASET_RANDOM, 1000, 8699U, 0U, 19952U},
  {"heap", DATASET_RANDOM, 1000, 16827U, 9083U, 27249U},
  {"bubble", DATASET_SORTED, 1000, 499500U, 0U, 0U},
  {"selection", DATASET_SORTED, 1000, 499500U, 0U, 0U},
  {"insertion", DATASET_SORTED, 1000, 999U, 0U, 1998U},
  {"quick", DATASET_SORTED, 1000, 48079U, 23494U, 71059U},
  {"merge", DATASET_SORTED, 1000, 5044U, 0U, 19952U},
  {"heap", DATASET_SORTED, 1000, 17583U, 9708U, 29124U},
  {"bubble", DATASET_REVERSED, 1000, 499500U, 499500U, 1498500U},
  {"selection", DATASET_REVERSED, 1000, 499500U, 500U, 1500U},
  {"insertion", DATASET_REVERSED, 1000, 499500U, 0U, 501498U},
  {"quick", DATASET_REVERSED, 1000, 36487U, 18152U, 55030U},
  {"merge", DATASET_REVERSED, 1000, 4932U, 0U, 19952U},
  {"heap", DATASET_REVERSED, 1000, 15965U, 8316U, 24948U},
  {"bubble", DATASET_NEARLY_SORTED, 1000, 499500U, 5740U, 17220U},
  {"selection", DATASET_NEARLY_SORTED, 1000, 499500U, 10U, 30U},
  {"insertion", DATASET_NEARLY_SORTED, 1000, 6739U, 0U, 7738U},
  {"quick", DATASET_NEARLY_SORTED, 1000, 24999U, 12352U, 37625U},
  {"merge", DATASET_NEARLY_SORTED, 1000, 6779U, 0U, 19952U},
  {"heap", DATASET_NEARLY_SORTED, 1000, 17571U, 9684U, 29052U},
  {"bubble", DATASET_FEW_UNIQUE, 1000, 499500U, 222170U, 666510U},
  {"selection", DATASET_FEW_UNIQUE, 1000, 499500U, 901U, 2703U},
  {"insertion", DATASET_FEW_UNIQUE, 1000, 223167U, 0U, 224168U},
  {"quick", DATASET_FEW_UNIQUE, 1000, 4940U, 2008U, 6034U},
  {"merge", DATASET_FEW_UNIQUE, 1000, 8503U, 0U, 19952U},
  {"heap", DATASET_FEW_UNIQUE, 1000, 15927U, 8363U, 25089U},
};

static const SortAlgorithm *find_sort_algorithm(const char *name) {
  size_t count = 0U;
  const SortAlgorithm *algorithms = get_sort_algorithms(&count);

  for (size_t i = 0; i < count; i++) {
    if (strcmp(algorithms[i].name, name) == 0) {
      return &algorithms[i];
    }
  }

  return NULL;
}

static int check_golden_entry(const GoldenCounters *golden) {
  const SortAlgorithm *sorter = find_sort_algorithm(golden->algorithm);
  Metrics metrics;
  Rng rng;
  Image *images = malloc((size_t)golden->size * sizeof(Image));
  Image *temp = malloc((size_t)golden->size * sizeof(Image));
  int ok = 0;

  if (sorter && images && temp) {
    rng_seed(&rng, GOLDEN_SEED);
    generate_images(images, golden->size, &rng, golden->dataset);
    sorter->sort(images, golden->size, temp, &metrics);

    ok = validate_sorted_images(images, golden->size) &&
         metrics.comparisons == golden->comparisons &&
         metrics.swaps == golden->swaps &&
         metrics.movements == golden->movements;

    if (!ok) {
      fprintf(stderr,
              "Falha golden: %s/%s n=%d esperado cmp=%" PRIu64 " swp=%" PRIu64 " mov=%" PRIu64
              " obtido cmp=%" PRIu64 " swp=%" PRIu64 " mov=%" PRIu64 ".\n",
              golden->algorithm,
              dataset_type_name(golden->dataset),
              golden->size,
              golden->comparisons,
              golden->swaps,
              golden->movements,
              metrics.comparisons,
              metrics.swaps,
              metrics.movements);
    }
  } else {
    fprintf(stderr, "Falha golden: algoritmo '%s' ausente ou sem memoria.\n", golden->algorithm);
  }

  free(images);
  free(temp);
  return ok;
}

static int test_golden_counters(void) {
  int failures = 0;

  for (size_t i = 0; i < sizeof(golden_counters) / sizeof(golden_counters[0]); i++) {
    if (!check_golden_entry(&golden_counters[i])) {
      failures++;
    }
  }

  return failures == 0;
}

int main(void) {
  static const int single[] = {42};
  static const int sorted[] = {1, 2, 3, 4, 5, 6};
  static const int reversed[] = {6, 5, 4, 3, 2, 1};
  static const int duplicates[] = {7, 7, 7, 7, 7};
  static const int many_duplicates[] = {3, 1, 3, 2, 3, 1, 2, 2, 3, 1, 3};
  static const int mixed[] = {3, 1, 2, 3, 0, -1, 2};

  const TestCase cases[] = {
    {"empty", NULL, 0},
    {"single", single, (int)(sizeof(single) / sizeof(single[0]))},
    {"sorted", sorted, (int)(sizeof(sorted) / sizeof(sorted[0]))},
    {"reversed", reversed, (int)(sizeof(reversed) / sizeof(reversed[0]))},
    {"duplicates", duplicates, (int)(sizeof(duplicates) / sizeof(duplicates[0]))},
    {"many_duplicates", many_duplicates, (int)(sizeof(many_duplicates) / sizeof(many_duplicates[0]))},
    {"mixed", mixed, (int)(sizeof(mixed) / sizeof(mixed[0]))}
  };
  int failures = 0;
  int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));
  size_t sorter_count = 0U;
  const SortAlgorithm *sorters = get_sort_algorithms(&sorter_count);

  for (size_t s = 0; s < sorter_count; s++) {
    for (int c = 0; c < num_cases; c++) {
      if (!run_case(&sorters[s], &cases[c])) {
        failures++;
      }
    }

    if (!run_random_case(&sorters[s])) {
      failures++;
    }
  }

  if (!test_validation_rejects_duplicate_ids()) {
    fprintf(stderr, "Falha: validacao aceitou ids duplicados.\n");
    failures++;
  }

  if (!test_dataset_generation()) {
    failures++;
  }

  if (!test_dataset_parse()) {
    fprintf(stderr, "Falha: parse de datasets.\n");
    failures++;
  }

  if (!test_statistics_module()) {
    fprintf(stderr, "Falha: modulo de estatisticas.\n");
    failures++;
  }

  if (!test_registry_module()) {
    fprintf(stderr, "Falha: modulo de registry.\n");
    failures++;
  }

  if (!test_csv_writer_module()) {
    fprintf(stderr, "Falha: modulo csv_writer.\n");
    failures++;
  }

  if (!test_golden_counters()) {
    fprintf(stderr, "Falha: contadores golden divergentes.\n");
    failures++;
  }

  if (failures != 0) {
    fprintf(stderr, "%d teste(s) falharam.\n", failures);
    return 1;
  }

  printf("Todos os testes de ordenacao passaram.\n");
  return 0;
}
