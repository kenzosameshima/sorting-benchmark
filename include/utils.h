#include <stddef.h>

#ifndef UTILS_H
#define UTILS_H

// Reads integers from a file into a dynamically allocated array
// Returns pointer to the array and sets *size to the number of elements read
// Returns NULL on failure
int* load_int_array_from_file(const char* filename, size_t* size);

// Frees arrays allocated by load_int_array_from_file
void free_int_array(int* arr);

/*
// Checks if an array is sorted in non-decreasing order.
int is_sorted(int arr[], int size);

// Prints an array (for debugging or small samples).
void print_array(int arr[], int size);
*/

#endif
