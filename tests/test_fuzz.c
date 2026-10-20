/*
 * test_fuzz.c - stage 13: random engines started and driven on their own
 * crank stay numbers and don't run away (native/fuzz.h), at every quality.
 * A few short runs here keep the fuzzer working; make fuzz runs twenty of
 * 10 s each.
 */
#include "fuzz.h"
#include "rb.h"
#include "rb_test.h"

#include <stdio.h>

void test_fuzz_random_engines(void)
{
    for (unsigned long long seed = 1; seed <= 4; seed++) {
        rb_fuzz_result r;
        const int ok = rb_fuzz_run(seed, 1.5, 2, (int)(seed % RB_QUALITY_COUNT), &r);
        CHECK(ok);
        if (!ok) printf("    seed %llu: %s\n", seed, r.what);
        CHECK(r.seconds >= 1.5 && r.max_rpm > 0.0);
    }
}
