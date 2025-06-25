#include <stddef.h>
#include "../include/sort.h"

static void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void cocktailShakerSort(int arr[], size_t len, sort_stats *stats) {
    if (len == 0) return;

    size_t start = 0;
    size_t end = len - 1;
    int swapped = 1;

    while (swapped) {
        swapped = 0;
        size_t last_swap_idx = 0;

        for (size_t i = start; i < end; i++) {
            stats->comparisons++;
            if (arr[i] > arr[i + 1]) {
                swap(&arr[i], &arr[i + 1]);
                stats->swaps++;
                swapped = 1;
                last_swap_idx = i;
            }
        }
        end = last_swap_idx;

        if (!swapped) break;

        swapped = 0;
        last_swap_idx = 0;

        for (size_t i = end; i-- > start;) {
            stats->comparisons++;
            if (arr[i] > arr[i + 1]) {
                swap(&arr[i], &arr[i + 1]);
                stats->swaps++;
                swapped = 1;
                last_swap_idx = i;
            }
        }
        start = last_swap_idx + 1;
    }
}
