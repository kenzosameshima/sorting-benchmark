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
void quick_sort(Image arr[], int low, int high, Metrics *metrics);
void merge_sort(Image arr[], int left, int right, Image arr_copy[], Metrics *metrics);
void heap_sort(Image arr[], int n, Metrics *metrics);

#endif
