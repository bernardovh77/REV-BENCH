/*
 * csv.c - tiny CSV writer. Numbers use "%.9g": enough digits to tell
 * neighbouring float values apart, short when the value is round.
 */
#include "csv.h"

#include <stdio.h>
#include <stdlib.h>

struct csv_writer {
    FILE *f;
    int   ncols;
    int   failed;
};

csv_writer *csv_open(const char *path, const char *const *columns, int ncols)
{
    csv_writer *w;

    if (!path || !columns || ncols <= 0) return NULL;
    w = calloc(1, sizeof *w);
    if (!w) return NULL;

    /* Keep identical LF-delimited output on Windows and Unix. */
    w->f = fopen(path, "wb");
    if (!w->f) {
        free(w);
        return NULL;
    }
    w->ncols = ncols;

    for (int i = 0; i < ncols; i++)
        if (fprintf(w->f, "%s%s", i ? "," : "", columns[i] ? columns[i] : "") < 0)
            w->failed = 1;
    if (fputc('\n', w->f) == EOF)
        w->failed = 1;
    return w;
}

int csv_row(csv_writer *w, const double *values)
{
    if (!w || !values || w->failed) return -1;

    for (int i = 0; i < w->ncols; i++) {
        if (fprintf(w->f, "%s%.9g", i ? "," : "", values[i]) < 0) {
            w->failed = 1;
            return -1;
        }
    }
    if (fputc('\n', w->f) == EOF) {
        w->failed = 1;
        return -1;
    }
    return 0;
}

int csv_close(csv_writer *w)
{
    int rc;

    if (!w) return -1;
    rc = w->failed ? -1 : 0;
    if (fclose(w->f) != 0) rc = -1;
    free(w);
    return rc;
}
