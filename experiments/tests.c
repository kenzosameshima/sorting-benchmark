#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

#include "generator.h"
#include "sorting.h"
#include "tests.h"
#include "utils.h"

#define MIN_BENCH_SECONDS 0.100
#define DATASET_SEED_MIX 0x9E3779B9u

typedef void (*SortFunction)(Image arr[], int n, Metrics *metrics);

typedef struct {
  const char *name;
  SortFunction sort;
  int is_quadratic;
} Algorithm;

typedef struct {
  double median;
  double stddev;
  double min;
  double max;
} TimeStats;

static int get_repetitions_for_size(int n) {
  if (n <= 1000) {
    return 30;
  }

  if (n <= 10000) {
    return 10;
  }

  return 3;
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

static int compare_doubles(const void *left, const void *right) {
  double a = *(const double *)left;
  double b = *(const double *)right;

  return (a > b) - (a < b);
}

static TimeStats calculate_time_stats(const double times[], int count) {
  TimeStats stats = {0.0, 0.0, 0.0, 0.0};
  double sum = 0.0;
  double *sorted = malloc((size_t)count * sizeof(double));

  if (!sorted || count <= 0) {
    free(sorted);
    return stats;
  }

  stats.min = times[0];
  stats.max = times[0];
  for (int i = 0; i < count; i++) {
    sorted[i] = times[i];
    sum += times[i];
    if (times[i] < stats.min) {
      stats.min = times[i];
    }
    if (times[i] > stats.max) {
      stats.max = times[i];
    }
  }

  qsort(sorted, (size_t)count, sizeof(double), compare_doubles);
  if (count % 2 == 0) {
    stats.median = (sorted[count / 2 - 1] + sorted[count / 2]) / 2.0;
  } else {
    stats.median = sorted[count / 2];
  }

  double mean = sum / count;
  double variance = 0.0;
  for (int i = 0; i < count; i++) {
    double delta = times[i] - mean;
    variance += delta * delta;
  }
  stats.stddev = sqrt(variance / count);

  free(sorted);
  return stats;
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

static int ensure_output_directory(const char *path) {
  if (MKDIR(path) == 0 || errno == EEXIST) {
    return 1;
  }

  fprintf(stderr, "Erro ao criar diretorio de resultados: %s\n", path);
  return 0;
}

static void generate_datasets(Image datasets[], int n, int repetitions, uint32_t seed) {
  Rng rng;

  rng_seed(&rng, seed);

  for (int r = 0; r < repetitions; r++) {
    generate_random_images(&datasets[(size_t)r * (size_t)n], n, &rng);
  }
}

void run_experiments(uint32_t seed) {
  static const int SIZES[] = {1000, 10000, 100000};
  const int num_sizes = (int)(sizeof(SIZES) / sizeof(SIZES[0]));
  const int QUADRATIC_LIMIT = 10000;
  const Algorithm ALGORITHMS[] = {
    {"bubble", bubble_sort, 1},
    {"selection", selection_sort, 1},
    {"insertion", insertion_sort, 1},
    {"quick", quick_sort, 0},
    {"merge", merge_sort, 0},
    {"heap", heap_sort, 0}
  };

  const int num_algorithms = (int)(sizeof(ALGORITHMS) / sizeof(ALGORITHMS[0]));

  if (!ensure_output_directory("results")) {
    return;
  }

  FILE *file = fopen("results/results.csv", "w");
  if (!file) {
    fprintf(stderr, "Erro ao abrir arquivo de saida.\n");
    return;
  }

  fprintf(file,
          "algorithm,size,seed,repetitions,time_median,time_stddev,time_min,time_max,"
          "average_comparisons,average_swaps,average_movements\n");

  for (int s = 0; s < num_sizes; s++) {
    int n = SIZES[s];
    int repetitions = get_repetitions_for_size(n);
    size_t dataset_count;

    if (!checked_mul_size((size_t)repetitions, (size_t)n, &dataset_count)) {
      fprintf(stderr, "Overflow ao calcular tamanho dos datasets (n=%d).\n", n);
      continue;
    }

    Image *datasets = checked_malloc_array(dataset_count, sizeof(Image));
    Image *copy = checked_malloc_array((size_t)n, sizeof(Image));

    if (!datasets || !copy) {
      fprintf(stderr, "Erro de memoria ao alocar vetores (n=%d).\n", n);
      free(datasets);
      free(copy);
      continue;
    }

    generate_datasets(datasets, n, repetitions, seed ^ DATASET_SEED_MIX ^ (uint32_t)n);

    for (int a = 0; a < num_algorithms; a++) {
      double total_elapsed = 0.0;
      uint64_t total_comparisons = 0;
      uint64_t total_swaps = 0;
      uint64_t total_movements = 0;
      int executed_runs = 0;
      int allocated_runs = repetitions;
      double *times = checked_malloc_array((size_t)allocated_runs, sizeof(double));

      if (!times) {
        fprintf(stderr, "Erro de memoria ao registrar tempos (n=%d).\n", n);
        continue;
      }

      if (ALGORITHMS[a].is_quadratic && n > QUADRATIC_LIMIT) {
        fprintf(file, "%s,%d,%" PRIu32 ",%d,NA,NA,NA,NA,NA,NA,NA\n",
                ALGORITHMS[a].name,
                n,
                seed,
                0);
        fflush(file);
        printf("Pulando %s com n=%d (custo muito alto para O(n^2)).\n",
               ALGORITHMS[a].name,
               n);
        free(times);
        continue;
      }

      printf("Executando %s com n=%d (minimo %d repeticoes, >= %.0fms)...\n",
             ALGORITHMS[a].name,
             n,
             repetitions,
             MIN_BENCH_SECONDS * 1000.0);

      while (executed_runs < repetitions || total_elapsed < MIN_BENCH_SECONDS) {
        Metrics metrics;
        int dataset_index = executed_runs % repetitions;

        if (executed_runs == allocated_runs) {
          int new_allocated_runs = allocated_runs * 2;
          size_t new_times_size;
          double *new_times;

          if (allocated_runs > INT_MAX / 2 ||
              !checked_mul_size((size_t)new_allocated_runs, sizeof(double), &new_times_size)) {
            fprintf(stderr, "Overflow ao expandir tempos (n=%d).\n", n);
            break;
          }

          new_times = realloc(times, new_times_size);

          if (!new_times) {
            fprintf(stderr, "Erro de memoria ao expandir tempos (n=%d).\n", n);
            break;
          }

          times = new_times;
          allocated_runs = new_allocated_runs;
        }

        copy_images(copy, &datasets[(size_t)dataset_index * (size_t)n], n);

        double start = now_seconds();
        ALGORITHMS[a].sort(copy, n, &metrics);
        double end = now_seconds();

        if (!is_sorted_by_size(copy, n)) {
          fprintf(stderr,
                  "Falha na ordenacao: %s (n=%d, repeticao=%d).\n",
                  ALGORITHMS[a].name,
                  n,
                  executed_runs + 1);
          break;
        }

        times[executed_runs] = end - start;
        total_elapsed += times[executed_runs];
        total_comparisons += metrics.comparisons;
        total_swaps += metrics.swaps;
        total_movements += metrics.movements;
        executed_runs++;
      }

      if (executed_runs == 0) {
        fprintf(file, "%s,%d,%" PRIu32 ",%d,NA,NA,NA,NA,NA,NA,NA\n",
                ALGORITHMS[a].name,
                n,
                seed,
                0);
        fflush(file);
        free(times);
        continue;
      }

      TimeStats stats = calculate_time_stats(times, executed_runs);

      fprintf(file, "%s,%d,%" PRIu32 ",%d,%.9f,%.9f,%.9f,%.9f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
              ALGORITHMS[a].name,
              n,
              seed,
              executed_runs,
              stats.median,
              stats.stddev,
              stats.min,
              stats.max,
              total_comparisons / (uint64_t)executed_runs,
              total_swaps / (uint64_t)executed_runs,
              total_movements / (uint64_t)executed_runs);
      fflush(file);
      free(times);
    }

    free(datasets);
    free(copy);
  }

  fclose(file);
  printf("Experimentos finalizados. Resultado salvo em results/results.csv\n");
}
