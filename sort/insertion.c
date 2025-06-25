#include <stddef.h>
#include "../include/sort.h"

void insertionSort(int arr[], size_t len, sort_stats* stats) {
    for (size_t i = 1; i < len; i++) {
        int key = arr[i];
        int j = (int)i - 1;

        while (j >= 0) {
            stats->comparisons++;
            if (arr[j] > key) {
                arr[j + 1] = arr[j];
                j--;
            } else {
                break;
            }
        }

        arr[j + 1] = key;
        stats->swaps++;
    }
}
