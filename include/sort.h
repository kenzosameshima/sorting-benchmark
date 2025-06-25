#ifndef SORT_H
#define SORT_H

#include "sort_stats.h"

void heapSort(int array[], size_t size, sort_stats* stats);
void mergeSort(int array[], size_t size, sort_stats* stats);
void insertionSort(int array[], size_t size, sort_stats* stats);
void selectionSort(int array[], size_t size, sort_stats* stats);
void cocktailShakerSort(int array[], size_t size, sort_stats* stats);
void quicksort_wrapper(int array[], size_t size, sort_stats* stats);

#endif
