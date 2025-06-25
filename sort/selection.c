#include <stddef.h>
#include "../include/sort.h"

static void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void selectionSort(int array[], size_t size, sort_stats* stats) {
    for (size_t i = 0; i < size - 1; i++) {
        size_t min_idx = i;
        for (size_t j = i + 1; j < size; j++) {
            stats->comparisons++;
            if (array[j] < array[min_idx]) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            int temp = array[i];
            array[i] = array[min_idx];
            array[min_idx] = temp;
            stats->swaps++;
        }
    }
}
