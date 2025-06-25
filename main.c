#include <stdio.h>
#include "sort.h"
#include "benchmark.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    size_t size;
    int* data = load_int_array_from_file(argv[1], &size);
    if (!data) {
        fprintf(stderr, "Error loading input file: %s\n", argv[1]);
        return 1;
    }

    typedef struct {
        const char* name;
        sort_function sort_func;
    } Algorithm;

    Algorithm algorithms[] = {
        {"Selection Sort", selectionSort},
        {"Insertion Sort", insertionSort},
        {"Cocktail Shaker Sort", cocktailShakerSort},
        {"Heap Sort", heapSort},
        {"Merge Sort", mergeSort},
        {"Quick Sort", quicksort_wrapper}
    };

    int num_algorithms = sizeof(algorithms) / sizeof(algorithms[0]);
    printf("Benchmarking file: %s (Elements: %zu)\n\n", argv[1], size);

    for (int i = 0; i < num_algorithms; i++) {
        sort_stats stats = {0};
        double elapsed = benchmark_sort(algorithms[i].sort_func, data, size, &stats);
        if (elapsed < 0) {
            printf("%-22s: Error during benchmark\n", algorithms[i].name);
        } else {
            printf("%-22s | %10.6f s | %15zu comparisons | %15zu swaps\n",
                algorithms[i].name, elapsed, stats.comparisons, stats.swaps);

        }
    }


    free_int_array(data);
    
    return 0;
}
