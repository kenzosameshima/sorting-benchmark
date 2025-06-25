#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int* load_int_array_from_file(const char* filename, size_t* size) {
    if (!filename || !size) return NULL;
    *size = 0;

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    size_t capacity = 100;
    int* arr = malloc(capacity * sizeof(int));
    if (!arr) {
        fclose(file);
        return NULL;
    }

    size_t count = 0;
    while (fscanf(file, "%d", &arr[count]) == 1) {
        count++;
        if (count == capacity) {
            capacity *= 2;
            int* temp = realloc(arr, capacity * sizeof(int));
            if (!temp) {
                free(arr);
                fclose(file);
                return NULL;
            }
            arr = temp;
        }
    }

    fclose(file);
    *size = count;
    return arr;
}

void free_int_array(int* arr) {
    free(arr);
}
