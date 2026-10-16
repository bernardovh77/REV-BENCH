/*
 * csv.h - tiny CSV writer for logging simulation traces (native tools).
 *
 *   const char *cols[] = {"time_s", "rpm"};
 *   csv_writer *w = csv_open("renders/run.csv", cols, 2);
 *   double row[2] = {t, rpm};
 *   csv_row(w, row);
 *   csv_close(w);
 */
#ifndef RB_CSV_H
#define RB_CSV_H

typedef struct csv_writer csv_writer;

/* Writes the header line. Returns NULL on failure. */
csv_writer *csv_open(const char *path, const char *const *columns, int ncols);
/* Writes one row of exactly `ncols` values. Returns 0 or -1. */
int csv_row(csv_writer *w, const double *values);
/* Closes the file and frees `w`. Returns 0 or -1 if any write failed. */
int csv_close(csv_writer *w);

#endif /* RB_CSV_H */
