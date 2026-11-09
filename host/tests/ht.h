/*
 * ht.h - the host tests' small framework (stage 123), like tests/rb_test.h
 * but for tests that start threads: checks are made on the test's own thread
 * (a worker thread records what it saw and the test checks it after joining).
 *
 *   CHECK(cond)            the condition must be true
 *   CHECK_EQ_INT(a, b)     a == b, printing both when it fails
 *   CHECK_NEAR(a, b, tol)  |a - b| <= tol (fails on NaN)
 *
 * A failed check prints file:line and carries on. Tests are `void test_x(void)`
 * functions registered in host_tests_main.c; `rb_host_tests NAME...` runs the
 * tests whose name contains one of the words.
 */
#ifndef HT_H
#define HT_H

void ht_check(int ok, const char *expr, const char *file, int line);
void ht_check_int(long long a, long long b, const char *ea, const char *eb, const char *file, int line);
void ht_check_near(double a, double b, double tol, const char *ea, const char *eb, const char *file, int line);

#define CHECK(cond) ht_check((cond) ? 1 : 0, #cond, __FILE__, __LINE__)
#define CHECK_EQ_INT(a, b) ht_check_int((long long)(a), (long long)(b), #a, #b, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) ht_check_near((double)(a), (double)(b), (double)(tol), #a, #b, __FILE__, __LINE__)

#endif /* HT_H */
