#include <stdio.h>
#include <stdlib.h>

#include "sorting.h"
#include "utils.h"

typedef void (*TestSortFunction)(Image arr[], int n, Metrics *metrics);

typedef struct {
  const char *name;
  TestSortFunction sort;
} SortUnderTest;

typedef struct {
  const char *name;
  const int *values;
  int count;
} TestCase;

static void fill_images(Image images[], const int values[], int count) {
  if (count <= 0) {
    return;
  }

  for (int i = 0; i < count; i++) {
    images[i].id = i;
    images[i].size = values[i];
  }
}

static int same_items(const Image images[], int count, const Image expected[]) {
  int *seen;

  if (count < 0) {
    return 0;
  }

  if (count == 0) {
    return 1;
  }

  seen = calloc((size_t)count, sizeof(int));
  if (!seen) {
    return 0;
  }

  for (int i = 0; i < count; i++) {
    int found = 0;

    for (int j = 0; j < count; j++) {
      if (!seen[j] &&
          images[i].id == expected[j].id &&
          images[i].size == expected[j].size) {
        seen[j] = 1;
        found = 1;
        break;
      }
    }

    if (!found) {
      free(seen);
      return 0;
    }
  }

  free(seen);
  return 1;
}

static int run_case(const SortUnderTest *sorter, const TestCase *test_case) {
  Metrics metrics;
  Image *images = NULL;
  Image *expected = NULL;
  int ok;

  if (test_case->count > 0) {
    images = malloc((size_t)test_case->count * sizeof(Image));
    expected = malloc((size_t)test_case->count * sizeof(Image));
    if (!images || !expected) {
      fprintf(stderr, "Falha ao alocar caso de teste.\n");
      free(images);
      free(expected);
      return 0;
    }

    fill_images(images, test_case->values, test_case->count);
    copy_images(expected, images, test_case->count);
  }

  sorter->sort(images, test_case->count, &metrics);
  ok = is_sorted_by_size(images, test_case->count) &&
       same_items(images, test_case->count, expected);

  if (!ok) {
    fprintf(stderr, "Falha: %s em '%s'.\n", sorter->name, test_case->name);
  }

  free(images);
  free(expected);
  return ok;
}

int main(void) {
  static const int single[] = {42};
  static const int duplicates[] = {7, 7, 7, 7, 7};
  static const int sorted[] = {1, 2, 3, 4, 5, 6};
  static const int reversed[] = {6, 5, 4, 3, 2, 1};
  static const int small[] = {3, 1, 2, 3, 0, -1, 2};

  const TestCase cases[] = {
    {"empty", NULL, 0},
    {"single", single, (int)(sizeof(single) / sizeof(single[0]))},
    {"duplicates", duplicates, (int)(sizeof(duplicates) / sizeof(duplicates[0]))},
    {"sorted", sorted, (int)(sizeof(sorted) / sizeof(sorted[0]))},
    {"reversed", reversed, (int)(sizeof(reversed) / sizeof(reversed[0]))},
    {"small", small, (int)(sizeof(small) / sizeof(small[0]))}
  };
  const SortUnderTest sorters[] = {
    {"bubble_sort", bubble_sort},
    {"selection_sort", selection_sort},
    {"insertion_sort", insertion_sort},
    {"quick_sort", quick_sort},
    {"merge_sort", merge_sort},
    {"heap_sort", heap_sort}
  };

  int failures = 0;
  int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));
  int num_sorters = (int)(sizeof(sorters) / sizeof(sorters[0]));

  for (int s = 0; s < num_sorters; s++) {
    for (int c = 0; c < num_cases; c++) {
      if (!run_case(&sorters[s], &cases[c])) {
        failures++;
      }
    }
  }

  if (failures != 0) {
    fprintf(stderr, "%d teste(s) falharam.\n", failures);
    return 1;
  }

  printf("Todos os testes unitarios passaram.\n");
  return 0;
}
