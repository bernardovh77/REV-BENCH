/*
 * rb_test.h - a tiny unit-test framework with no dependencies.
 *
 *   CHECK(cond)            the condition must be true
 *   CHECK_NEAR(a, b, tol)  |a - b| <= tol   (fails on NaN)
 *   CHECK_EQ_INT(a, b)     a == b, printing both values when it fails
 *
 * A failed check prints file:line and carries on, so one run shows every
 * failure. Tests are plain `void test_xxx(void)` functions registered in
 * tests/test_main.c.
 */
#ifndef RB_TEST_H
#define RB_TEST_H

#include <stddef.h>

void rbt_check(int ok, const char *expr, const char *file, int line);
void rbt_check_near(double a, double b, double tol,
                    const char *ea, const char *eb, const char *file, int line);
void rbt_check_int(long long a, long long b,
                   const char *ea, const char *eb, const char *file, int line);

/* Path for a scratch file inside $RBT_TMPDIR (default: current folder). */
void rbt_tmp_path(char *buf, size_t size, const char *name);

#define CHECK(cond) \
    rbt_check((cond) ? 1 : 0, #cond, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) \
    rbt_check_near((double)(a), (double)(b), (double)(tol), #a, #b, __FILE__, __LINE__)
#define CHECK_EQ_INT(a, b) \
    rbt_check_int((long long)(a), (long long)(b), #a, #b, __FILE__, __LINE__)

#endif /* RB_TEST_H */
