#ifndef SORT_STATS_H
#define SORT_STATS_H

#include <stddef.h>

typedef struct {
    size_t comparisons;
    size_t swaps;
    size_t merges;       // Useful specifically for merge sort
    size_t heapify_calls; // For heap sort insights
    size_t recursion_depth; // For recursive sorts
    size_t allocations;  // For tracking dynamic memory use
    // Add more fields as needed
} sort_stats;

#endif // SORT_STATS_H
