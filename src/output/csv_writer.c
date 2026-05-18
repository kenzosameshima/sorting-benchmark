#include "csv_writer.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

#ifndef COMPILATION_FLAGS
#define COMPILATION_FLAGS "unknown"
#endif

static int ensure_output_directory(const char *path) {
  if (MKDIR(path) == 0 || errno == EEXIST) {
    return 1;
  }

  fprintf(stderr, "Erro ao criar diretorio de resultados: %s\n", path);
  return 0;
}

static void write_csv_field(FILE *file, const char *value) {
  fputc('"', file);
  for (const char *current = value; *current != '\0'; current++) {
    if (*current == '"') {
      fputc('"', file);
    }
    fputc(*current, file);
  }
  fputc('"', file);
}

int open_result_files(ResultFiles *files, const char *directory) {
  char runs_path[256];
  char summary_path[256];

  files->runs = NULL;
  files->summary = NULL;

  if (!ensure_output_directory(directory)) {
    return 0;
  }

  if (snprintf(runs_path, sizeof(runs_path), "%s/results.csv", directory) < 0 ||
      snprintf(summary_path, sizeof(summary_path), "%s/summary.csv", directory) < 0) {
    return 0;
  }

  files->runs = fopen(runs_path, "w");
  if (!files->runs) {
    fprintf(stderr, "Erro ao abrir arquivo de resultados detalhados.\n");
    return 0;
  }

  files->summary = fopen(summary_path, "w");
  if (!files->summary) {
    fprintf(stderr, "Erro ao abrir arquivo de resumo.\n");
    fclose(files->runs);
    files->runs = NULL;
    return 0;
  }

  return 1;
}

void close_result_files(ResultFiles *files) {
  if (files->runs) {
    fclose(files->runs);
    files->runs = NULL;
  }
  if (files->summary) {
    fclose(files->summary);
    files->summary = NULL;
  }
}

void write_result_headers(const ResultFiles *files) {
  fprintf(files->runs,
          "algorithm,size,rep,seed,dataset_type,timestamp,compiler,flags,status,time,"
          "comparisons,swaps,movements\n");
  fprintf(files->summary,
          "algorithm,size,seed,dataset_type,timestamp,compiler,flags,repetitions,status,"
          "time_mean,time_median,time_stddev,time_min,time_max,average_comparisons,average_swaps,"
          "average_movements\n");
}

void write_run_row(FILE *file,
                   const SortAlgorithm *algorithm,
                   int n,
                   int repetition,
                   uint32_t seed,
                   const char *dataset_type,
                   const char *timestamp,
                   const char *status,
                   double elapsed_seconds,
                   const Metrics *metrics) {
  write_csv_field(file, algorithm->name);
  fprintf(file, ",%d,%d,%" PRIu32 ",", n, repetition, seed);
  write_csv_field(file, dataset_type);
  fputc(',', file);
  write_csv_field(file, timestamp);
  fputc(',', file);
  write_csv_field(file, __VERSION__);
  fputc(',', file);
  write_csv_field(file, COMPILATION_FLAGS);
  fputc(',', file);
  write_csv_field(file, status);
  fprintf(file,
          ",%.9f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
          elapsed_seconds,
          metrics->comparisons,
          metrics->swaps,
          metrics->movements);
}

void write_skipped_run_row(FILE *file,
                           const SortAlgorithm *algorithm,
                           int n,
                           int repetition,
                           uint32_t seed,
                           const char *dataset_type,
                           const char *timestamp,
                           const char *status) {
  write_csv_field(file, algorithm->name);
  fprintf(file, ",%d,%d,%" PRIu32 ",", n, repetition, seed);
  write_csv_field(file, dataset_type);
  fputc(',', file);
  write_csv_field(file, timestamp);
  fputc(',', file);
  write_csv_field(file, __VERSION__);
  fputc(',', file);
  write_csv_field(file, COMPILATION_FLAGS);
  fputc(',', file);
  write_csv_field(file, status);
  fprintf(file, ",NA,NA,NA,NA\n");
}

void write_summary_row(FILE *file,
                       const SortAlgorithm *algorithm,
                       const AlgorithmResult *result,
                       int n,
                       uint32_t seed,
                       const char *dataset_type,
                       const char *timestamp) {
  write_csv_field(file, algorithm->name);
  fprintf(file, ",%d,%" PRIu32 ",", n, seed);
  write_csv_field(file, dataset_type);
  fputc(',', file);
  write_csv_field(file, timestamp);
  fputc(',', file);
  write_csv_field(file, __VERSION__);
  fputc(',', file);
  write_csv_field(file, COMPILATION_FLAGS);

  if (result->skipped || result->runs == 0) {
    fprintf(file, ",0,");
    write_csv_field(file, "SKIPPED");
    fprintf(file, ",NA,NA,NA,NA,NA,NA,NA,NA\n");
    return;
  }

  TimeStats stats = calculate_time_stats(result->times, result->runs);

  fprintf(file,
          ",%d,",
          result->runs);
  write_csv_field(file, "OK");
  fprintf(file,
          ",%.9f,%.9f,%.9f,%.9f,%.9f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
          stats.mean,
          stats.median,
          stats.stddev,
          stats.min,
          stats.max,
          result->comparisons / (uint64_t)result->runs,
          result->swaps / (uint64_t)result->runs,
          result->movements / (uint64_t)result->runs);
}
