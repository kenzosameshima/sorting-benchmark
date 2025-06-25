#include <stdlib.h>
#include <string.h>
#include "../include/sort.h"

static void merge(int from[], int to[], size_t left, size_t mid, size_t right, sort_stats *stats) {
    size_t i = left, j = mid + 1, k = left;

    stats->merges++;

    while (i <= mid && j <= right) {
        stats->comparisons++;
        if (from[i] <= from[j]) to[k++] = from[i++];
        else to[k++] = from[j++];
    }
    while (i <= mid) to[k++] = from[i++];
    while (j <= right) to[k++] = from[j++];
}

static void mergeSortRecursive(int from[], int to[], size_t left, size_t right, int depth, sort_stats *stats) {
    if (left >= right) {
        if (depth % 2 == 1)
            to[left] = from[left];
        return;
    }

    size_t mid = left + (right - left) / 2;

    mergeSortRecursive(to, from, left, mid, depth + 1, stats);
    mergeSortRecursive(to, from, mid + 1, right, depth + 1, stats);

    stats->comparisons++;
    if (from[mid] <= from[mid + 1]) {
        if (depth % 2 == 1) {
            memcpy(to + left, from + left, (right - left + 1) * sizeof(int));
        }
        return;
    }

    merge(from, to, left, mid, right, stats);
}

void mergeSort(int arr[], size_t len, sort_stats *stats) {
    if (len == 0) return;

    stats->allocations++;

    int* aux = malloc(len * sizeof(int));
    if (!aux) return;

    memcpy(aux, arr, len * sizeof(int));
    mergeSortRecursive(aux, arr, 0, len - 1, 0, stats);

    free(aux);
}
