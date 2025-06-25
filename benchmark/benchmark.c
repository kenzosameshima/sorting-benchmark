#include "benchmark.h"
#include "sort.h"
#include "sort_stats.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>

double benchmark_sort(sort_function sort_func, const int* arr, size_t size, sort_stats* stats) {
    int* copy = malloc(size * sizeof(int));
    if (!copy) return -1.0;

    memcpy(copy, arr, size * sizeof(int));

    if (stats) {
        memset(stats, 0, sizeof(sort_stats));
    }

    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    sort_func(copy, size, stats);

    QueryPerformanceCounter(&end);
    free(copy);

    return (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    // Or return milliseconds if you prefer:
    // return 1000.0 * (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
}
