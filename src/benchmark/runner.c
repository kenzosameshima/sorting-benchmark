#define _POSIX_C_SOURCE 200809L

#include "benchmark.h"

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "csv_writer.h"
#include "sort_algorithm.h"
#include "statistics.h"
#include "validation.h"

static int get_repetitions_for_size(int n) {
  if (n <= 1000) {
    return 30;
  }

  if (n <= 10000) {
    return 10;
  }

  return 3;
}

static int should_run_size(int n, int max_size) {
  return max_size <= 0 || n <= max_size;
}

static double now_seconds(void) {
#ifdef _WIN32
  static LARGE_INTEGER frequency;
  LARGE_INTEGER counter;

  if (frequency.QuadPart == 0) {
    QueryPerformanceFrequency(&frequency);
  }

  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

static int checked_mul_size(size_t a, size_t b, size_t *out) {
  if (a != 0U && b > SIZE_MAX / a) {
    return 0;
  }

  *out = a * b;
  return 1;
}

static void *checked_malloc_array(size_t count, size_t element_size) {
  size_t bytes;

  if (!checked_mul_size(count, element_size, &bytes)) {
    return NULL;
  }

  return malloc(bytes);
}

static void format_timestamp(char buffer[], size_t size) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  if (!tm_info || strftime(buffer, size, "%Y-%m-%dT%H:%M:%S%z", tm_info) == 0U) {
    if (size > 0U) {
      buffer[0] = '\0';
    }
  }
}

static int run_size(const BenchmarkConfig *config,
                    const SortAlgorithm algorithms[],
                    int algorithm_count,
                    int n,
                    DatasetType dataset_type,
                    Rng *rng,
                    const char *timestamp,
                    const ResultFiles *files) {
  int repetitions = config->repetitions > 0 ? config->repetitions : get_repetitions_for_size(n);
  const char *dataset_name = dataset_type_name(dataset_type);
  Image *base_dataset = checked_malloc_array((size_t)n, sizeof(Image));
  Image *work_array = checked_malloc_array((size_t)n, sizeof(Image));
  Image *temp_buffer = checked_malloc_array((size_t)n, sizeof(Image));
  AlgorithmResult *results = checked_malloc_array((size_t)algorithm_count, sizeof(*results));

  if (!base_dataset || !work_array || !temp_buffer || !results) {
    fprintf(stderr, "Failed to allocate memory for benchmark (n=%d).\n", n);
    free(base_dataset);
    free(work_array);
    free(temp_buffer);
    free(results);
    return 0;
  }

  if (!initialize_algorithm_results(results, algorithm_count, repetitions)) {
    fprintf(stderr, "Failed to allocate benchmark results (n=%d).\n", n);
    free(base_dataset);
    free(work_array);
    free(temp_buffer);
    free(results);
    return 0;
  }

  for (int a = 0; a < algorithm_count; a++) {
    if (algorithms[a].is_quadratic && n > config->skip_quadratic_after) {
      results[a].skipped = 1;
    }
  }

  printf("Running dataset=%s n=%d with %d repetitions and seed %" PRIu32 "...\n",
         dataset_name,
         n,
         repetitions,
         config->seed);

  for (int rep = 0; rep < repetitions; rep++) {
    generate_images(base_dataset, n, rng, dataset_type);

    for (int a = 0; a < algorithm_count; a++) {
      Metrics metrics;
      double start;
      double end;

      if (results[a].skipped) {
        write_skipped_run_row(files->runs,
                              &algorithms[a],
                              n,
                              rep + 1,
                              config->seed,
                              dataset_name,
                              timestamp,
                              "SKIPPED");
        continue;
      }

      memcpy(work_array, base_dataset, (size_t)n * sizeof(Image));

      start = now_seconds();
      algorithms[a].sort(work_array, n, temp_buffer, &metrics);
      end = now_seconds();

      if (!validate_sorted_images(work_array, n)) {
        fprintf(stderr,
                "Validation failed: %s (dataset=%s, n=%d, repetition=%d).\n",
                algorithms[a].name,
                dataset_name,
                n,
                rep + 1);
        results[a].skipped = 1;
        write_skipped_run_row(files->runs,
                              &algorithms[a],
                              n,
                              rep + 1,
                              config->seed,
                              dataset_name,
                              timestamp,
                              "INVALID");
        continue;
      }

      write_run_row(files->runs,
                    &algorithms[a],
                    n,
                    rep + 1,
                    config->seed,
                    dataset_name,
                    timestamp,
                    "OK",
                    end - start,
                    &metrics);

      results[a].times[results[a].runs] = end - start;
      results[a].comparisons += metrics.comparisons;
      results[a].swaps += metrics.swaps;
      results[a].movements += metrics.movements;
      results[a].runs++;
    }
  }

  for (int a = 0; a < algorithm_count; a++) {
    write_summary_row(files->summary,
                      &algorithms[a],
                      &results[a],
                      n,
                      config->seed,
                      dataset_name,
                      timestamp);
  }
  fflush(files->runs);
  fflush(files->summary);

  free_algorithm_results(results, algorithm_count);
  free(results);
  free(base_dataset);
  free(work_array);
  free(temp_buffer);
  return 1;
}

void run_experiments(const BenchmarkConfig *config) {
  static const int default_sizes[] = {100, 1000, 10000, 100000};
  static const DatasetType all_datasets[] = {
    DATASET_RANDOM,
    DATASET_SORTED,
    DATASET_REVERSED,
    DATASET_NEARLY_SORTED,
    DATASET_FEW_UNIQUE,
    DATASET_ORGAN_PIPE
  };
  const int *sizes = config->sizes ? config->sizes : default_sizes;
  int size_count = config->sizes ? config->size_count :
                                  (int)(sizeof(default_sizes) / sizeof(default_sizes[0]));
  const DatasetType *datasets = config->dataset_type == DATASET_ALL ? all_datasets :
                                                               &config->dataset_type;
  int dataset_count = config->dataset_type == DATASET_ALL ?
      (int)(sizeof(all_datasets) / sizeof(all_datasets[0])) : 1;
  size_t algorithm_count_size = 0U;
  const SortAlgorithm *algorithms = get_sort_algorithms(&algorithm_count_size);
  int algorithm_count;
  char timestamp[32];
  ResultFiles files;
  Rng rng;

  if (algorithm_count_size > (size_t)INT_MAX) {
    fprintf(stderr, "Quantidade de algoritmos excede limite suportado.\n");
    return;
  }
  algorithm_count = (int)algorithm_count_size;

  if (!open_result_files(&files, "results")) {
    return;
  }

  format_timestamp(timestamp, sizeof(timestamp));
  rng_seed(&rng, config->seed);
  write_result_headers(&files);

  for (int d = 0; d < dataset_count; d++) {
    for (int s = 0; s < size_count; s++) {
      int n = sizes[s];

      if (!should_run_size(n, config->max_size)) {
        continue;
      }

      (void)run_size(config, algorithms, algorithm_count, n, datasets[d], &rng, timestamp, &files);
    }
  }

  close_result_files(&files);
  printf("Experimentos finalizados. Resultados salvos em results/results.csv e results/summary.csv\n");
}
