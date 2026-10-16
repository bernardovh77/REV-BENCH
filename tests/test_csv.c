/*
 * test_csv.c - the CSV writer produces exactly the expected text.
 */
#include "csv.h"
#include "rb_test.h"

#include <stdio.h>
#include <string.h>

void test_csv_basic(void)
{
    char path[512], text[256];
    const char *cols[] = { "t", "x" };
    const double r1[] = { 0.0, 1.0 };
    const double r2[] = { 0.5, -2.0 };
    const double r3[] = { 1e-12, 123456789.0 };
    csv_writer *w;
    size_t n;
    FILE *f;

    rbt_tmp_path(path, sizeof path, "rbt_basic.csv");
    w = csv_open(path, cols, 2);
    CHECK(w != NULL);
    if (!w) return;
    CHECK_EQ_INT(csv_row(w, r1), 0);
    CHECK_EQ_INT(csv_row(w, r2), 0);
    CHECK_EQ_INT(csv_row(w, r3), 0);
    CHECK_EQ_INT(csv_close(w), 0);

    f = fopen(path, "rb");
    CHECK(f != NULL);
    if (!f) return;
    n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = '\0';
    CHECK(strcmp(text, "t,x\n0,1\n0.5,-2\n1e-12,123456789\n") == 0);

    CHECK(csv_open(path, cols, 0) == NULL);
    CHECK(csv_row(NULL, r1) == -1);
    remove(path);
}
