#include <stddef.h>
#include "../include/sort.h"

void heapify(int arr[], size_t len, size_t idx, sort_stats *stats) {
    int temp = arr[idx];
    size_t left = 2 * idx + 1;

    while (left < len) {
        size_t right = left + 1;
        size_t child;

        if (right < len) {
            stats->comparisons++;
            child = (arr[right] > arr[left]) ? right : left;
        } else {
            child = left;
        }

        stats->comparisons++;
        if (arr[child] <= temp) {
            break;
        }

        arr[idx] = arr[child];
        stats->swaps++;

        idx = child;
        left = 2 * idx + 1;
    }

    if (arr[idx] != temp) {
        arr[idx] = temp;
        stats->swaps++;
    }
}

void heapSort(int arr[], size_t len, sort_stats *stats) {
    if (len == 0) return;

    for (size_t i = len / 2; i-- > 0;) {
        heapify(arr, len, i, stats);
    }

    for (size_t i = len - 1; i > 0; i--) {
        int temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;
        stats->swaps++;

        heapify(arr, i, 0, stats);
    }
}
