/*
 * test_cores.c - the render thread on the fast cores (stage 066's gap, fixed
 * before it): which logical processors a hybrid CPU's render thread is kept
 * to, and that the running thread really is.
 */
#include "ht.h"

#include <string.h>

#include "cores.h"
#include "host_test_util.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void test_cores_pick_only_the_fastest_class(void)
{
    /* Bernardo's i7-1355U as Windows reports it: 8 efficiency threads (class
     * 0), 4 performance threads (class 1, two cores with two threads each) */
    const unsigned char laptop[12] = { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    int picked[12];
    CHECK_EQ_INT(rbh_pick_fast_cores(laptop, 12, picked), 4);
    for (int i = 0; i < 4; i++) CHECK_EQ_INT(picked[i], i);

    /* three classes: only the top one */
    const unsigned char three[6] = { 0, 2, 1, 2, 0, 1 };
    CHECK_EQ_INT(rbh_pick_fast_cores(three, 6, picked), 2);
    CHECK_EQ_INT(picked[0], 1);
    CHECK_EQ_INT(picked[1], 3);

    /* every core alike (CI's runners, most desktops): no restriction at all */
    const unsigned char alike[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    CHECK_EQ_INT(rbh_pick_fast_cores(alike, 8, picked), 0);
    const unsigned char alike1[4] = { 3, 3, 3, 3 };
    CHECK_EQ_INT(rbh_pick_fast_cores(alike1, 4, picked), 0);

    /* nothing known */
    CHECK_EQ_INT(rbh_pick_fast_cores(laptop, 0, picked), 0);
    CHECK_EQ_INT(rbh_pick_fast_cores(NULL, 4, picked), 0);
}

#ifdef _WIN32
/* The machine's own answer: how many logical processors are in the top
 * efficiency class, and how many there are, or 0 fast when all are alike. */
static void machine_cores(int *fast, int *all)
{
    static unsigned char buffer[65536];
    ULONG length = 0;
    *fast = *all = 0;
    if (!GetSystemCpuSetInformation((PSYSTEM_CPU_SET_INFORMATION)buffer, sizeof buffer, &length,
                                    GetCurrentProcess(), 0))
        return;
    int count[256] = { 0 }, top = -1, lowest = 256;
    for (ULONG at = 0; at < length;) {
        const SYSTEM_CPU_SET_INFORMATION *e = (const SYSTEM_CPU_SET_INFORMATION *)(buffer + at);
        if (e->Type == CpuSetInformation) {
            const int c = e->CpuSet.EfficiencyClass;
            count[c]++;
            (*all)++;
            if (c > top) top = c;
            if (c < lowest) lowest = c;
        }
        at += e->Size;
    }
    *fast = (top > lowest) ? count[top] : 0;
}
#endif

void test_cores_render_thread_runs_on_the_fast_cores(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    CHECK_EQ_INT(rb_host_start(h), RB_HOST_OK);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
#ifdef _WIN32
    int fast = 0, all = 0;
    machine_cores(&fast, &all);
    CHECK(all > 0);
    CHECK_EQ_INT(s.priority.all_cores, all);
    /* read back from the thread itself: on a hybrid CPU only the fast ones */
    CHECK_EQ_INT(s.priority.fast_cores, fast);
    if (fast) CHECK(strstr(s.priority.policy, "fast cores") != NULL);
#else
    CHECK_EQ_INT(s.priority.fast_cores, 0);      /* stage 126 */
#endif
    rb_host_stop(h);
    rb_host_destroy(h);
}
