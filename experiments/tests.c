#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
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

typedef void (*SortFunction)(Image arr[], int n, Metrics *metrics);

typedef struct {
  const char *name;
  SortFunction sort;
  int is_quadratic;
} Algorithm;

static int get_repetitions_for_size(int n) {
  if (n <= 1000) {
    return 30;
  }

  if (n <= 10000) {
    return 10;
  }

  return 3;
}

static int ensure_output_directory(const char *path) {
  if (MKDIR(path) == 0 || errno == EEXIST) {
    return 1;
  }

  fprintf(stderr, "Erro ao criar diretorio de resultados: %s\n", path);
  return 0;
}

static void quick_sort_wrapper(Image arr[], int n, Metrics *metrics) {
  if (n <= 0) {
    reset_metrics(metrics);
    return;
  }

  quick_sort(arr, 0, n - 1, metrics);
}

static void merge_sort_wrapper(Image arr[], int n, Metrics *metrics) {
  if (n <= 0) {
    reset_metrics(metrics);
    return;
  }

  Image *aux = malloc((size_t)n * sizeof(Image));
  if (!aux) {
    fprintf(stderr, "Erro de memoria no merge_sort (n=%d).\n", n);
    reset_metrics(metrics);
    return;
  }

  merge_sort(arr, 0, n - 1, aux, metrics);
  free(aux);
}

void run_experiments(void) {
  static const int SIZES[] = {1000, 10000, 100000};
  const int num_sizes = (int)(sizeof(SIZES) / sizeof(SIZES[0]));
  const int QUADRATIC_LIMIT = 10000;
  const Algorithm ALGORITHMS[] = {
    {"bubble", bubble_sort, 1},
    {"selection", selection_sort, 1},
    {"insertion", insertion_sort, 1},
    {"quick", quick_sort_wrapper, 0},
    {"merge", merge_sort_wrapper, 0},
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
          "algorithm,size,repetitions,average_time,average_comparisons,average_movements\n");

  for (int s = 0; s < num_sizes; s++) {
    int n = SIZES[s];
    Image *original = malloc((size_t)n * sizeof(Image));
    Image *copy = malloc((size_t)n * sizeof(Image));

    if (!original || !copy) {
      fprintf(stderr, "Erro de memoria ao alocar vetores (n=%d).\n", n);
      free(original);
      free(copy);
      continue;
    }

    for (int a = 0; a < num_algorithms; a++) {
      int repetitions = get_repetitions_for_size(n);
      double total_elapsed = 0.0;
      long long total_comparisons = 0;
      long long total_movements = 0;
      int executed_runs = 0;

      if (ALGORITHMS[a].is_quadratic && n > QUADRATIC_LIMIT) {
        fprintf(file, "%s,%d,%d,NA,NA,NA\n",
                ALGORITHMS[a].name,
                n,
                0);
        fflush(file);
        printf("Pulando %s com n=%d (custo muito alto para O(n^2)).\n",
               ALGORITHMS[a].name,
               n);
        continue;
      }

      printf("Executando %s com n=%d (%d repeticoes)...\n",
              ALGORITHMS[a].name,
              n,
              repetitions);

      for (int r = 0; r < repetitions; r++) {
        Metrics metrics;

        generate_random_images(original, n);
        copy_images(copy, original, n);

        clock_t start = clock();
        ALGORITHMS[a].sort(copy, n, &metrics);
        clock_t end = clock();

        if (!is_sorted_by_size(copy, n)) {
          fprintf(stderr,
                  "Falha na ordenacao: %s (n=%d, repeticao=%d).\n",
                  ALGORITHMS[a].name,
                  n,
                  r + 1);
          continue;
        }

        total_elapsed += (double)(end - start) / CLOCKS_PER_SEC;
        total_comparisons += metrics.comparisons;
        total_movements += metrics.movements;
        executed_runs++;
      }

      if (executed_runs == 0) {
        fprintf(file, "%s,%d,%d,NA,NA,NA\n",
                ALGORITHMS[a].name,
                n,
                0);
        fflush(file);
        continue;
      }

      fprintf(file, "%s,%d,%d,%.6f,%lld,%lld\n",
              ALGORITHMS[a].name,
              n,
              executed_runs,
              total_elapsed / executed_runs,
              total_comparisons / executed_runs,
              total_movements / executed_runs);
      fflush(file);
    }

    free(original);
    free(copy);
  }

  fclose(file);
  printf("Experimentos finalizados. Resultado salvo em results/results.csv\n");
}
