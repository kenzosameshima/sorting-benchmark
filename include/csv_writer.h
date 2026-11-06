#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <stdint.h>
#include <stdio.h>

#include "sort_algorithm.h"
#include "statistics.h"

typedef struct {
  FILE *runs;
  FILE *summary;
} ResultFiles;

int open_result_files(ResultFiles *files, const char *directory);
void close_result_files(ResultFiles *files);
int write_dataset_csv(const char *directory, const char *dataset, int n, const Image images[]);
void write_result_headers(const ResultFiles *files);
void write_run_row(FILE *file,
                   const SortAlgorithm *algorithm,
                   int n,
                   int repetition,
                   uint32_t seed,
                   const char *dataset_type,
                   const char *timestamp,
                   const char *status,
                   double elapsed_seconds,
                   const Metrics *metrics);
void write_skipped_run_row(FILE *file,
                           const SortAlgorithm *algorithm,
                           int n,
                           int repetition,
                           uint32_t seed,
                           const char *dataset_type,
                           const char *timestamp,
                           const char *status);
void write_summary_row(FILE *file,
                       const SortAlgorithm *algorithm,
                       const AlgorithmResult *result,
                       int n,
                       uint32_t seed,
                       const char *dataset_type,
                       const char *timestamp);

#endif
