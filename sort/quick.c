#include <stddef.h>
#include "../include/sort.h"

static void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

static int medianOfThree(int arr[], int low, int high, sort_stats *stats) {
    int mid = low + (high - low) / 2;

    stats->comparisons++;
    if (arr[low] > arr[mid]) {
        swap(&arr[low], &arr[mid]);
        stats->swaps++;
    }

    stats->comparisons++;
    if (arr[low] > arr[high]) {
        swap(&arr[low], &arr[high]);
        stats->swaps++;
    }

    stats->comparisons++;
    if (arr[mid] > arr[high]) {
        swap(&arr[mid], &arr[high]);
        stats->swaps++;
    }

    if (mid != high && arr[mid] != arr[high]) {
        swap(&arr[mid], &arr[high]);
        stats->swaps++;
    }

    return arr[high];
}

static int partition(int arr[], int low, int high, sort_stats *stats) {
    int pivot = medianOfThree(arr, low, high, stats);
    int i = low - 1;

    for (int j = low; j < high; j++) {
        stats->comparisons++;
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
            stats->swaps++;
        }
    }

    swap(&arr[i + 1], &arr[high]);
    stats->swaps++;

    return i + 1;
}

static void quicksort(int arr[], int low, int high, sort_stats *stats) {
    while (low < high) {
        int pi = partition(arr, low, high, stats);

        if (pi - low < high - pi) {
            quicksort(arr, low, pi - 1, stats);
            low = pi + 1;
        } else {
            quicksort(arr, pi + 1, high, stats);
            high = pi - 1;
        }
    }
}

void quicksort_wrapper(int arr[], size_t len, sort_stats *stats) {
    if (len > 0) {
        quicksort(arr, 0, (int)len - 1, stats);
    }
}
