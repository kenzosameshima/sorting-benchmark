#include <stdio.h>
#include <stdlib.h>

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

  if (failures != 0) {
    fprintf(stderr, "%d teste(s) falharam.\n", failures);
    return 1;
  }

  printf("Todos os testes de ordenacao passaram.\n");
  return 0;
}
