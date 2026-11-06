#ifndef SORTING_H
#define SORTING_H

#include "metrics.h"

// Struct para simular imagens de satelite
typedef struct {
  int id;
  int size;  // criterio de ordenacao
} Image;

// Prototipos dos algoritmos
void bubble_sort(Image arr[], int n, Metrics *metrics);
void selection_sort(Image arr[], int n, Metrics *metrics);
void insertion_sort(Image arr[], int n, Metrics *metrics);
void quick_sort(Image arr[], int n, Metrics *metrics);
void quick_sort_naive(Image arr[], int n, Metrics *metrics);
void merge_sort(Image arr[], int n, Image temp[], Metrics *metrics);
void heap_sort(Image arr[], int n, Metrics *metrics);
void radix_sort(Image arr[], int n, Image temp[], Metrics *metrics);
void shell_sort(Image arr[], int n, Metrics *metrics);
void qsort_libc(Image arr[], int n, Metrics *metrics);

#endif
