#ifndef UTILS_H
#define UTILS_H

#include "sorting.h"

void copy_images(Image dest[], const Image src[], int n);
int is_sorted_by_size(const Image images[], int n);
int has_same_items(const Image original[], const Image sorted[], int n);

#endif
