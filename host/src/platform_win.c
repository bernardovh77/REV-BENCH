/*
 * platform_win.c - the host's Windows layer (stage 123).
 *
 * The render thread asks MMCSS (the Multimedia Class Scheduler Service) for
 * the "Pro Audio" task, which gives it a scheduling class that keeps it
 * running on time while ordinary threads compete. If MMCSS isn't there, the
 * thread falls back to the highest ordinary priority and says so in its
 * policy string, so the diagnostics never claim more than the system gave.
 */
#include "platform.h"

#include "audit.h"
#include "cores.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <avrt.h>
#include <psapi.h>
#include <mmsystem.h>

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "psapi.lib")
#endif

struct rbh_thread {
    HANDLE        handle;
    rbh_thread_fn fn;
    void         *arg;
};

struct rbh_sem {
    HANDLE handle;
};

struct rbh_mutex {
    SRWLOCK lock;
};

/* The MMCSS task handle of the thread that entered (one render thread). */
static _Thread_local HANDLE t_mmcss;
static _Thread_local int t_timer_raised;

static DWORD WINAPI thread_main(LPVOID p)
{
    rbh_thread *t = (rbh_thread *)p;
    t->fn(t->arg);
    return 0;
}

rbh_thread *rbh_plat_thread_start(rbh_thread_fn fn, void *arg)
{
    rbh_thread *t = (rbh_thread *)rbh_alloc(sizeof *t);
    if (!t) return NULL;
    t->fn = fn;
    t->arg = arg;
    t->handle = CreateThread(NULL, 0, thread_main, t, 0, NULL);
    if (!t->handle) {
        rbh_free(t);
        return NULL;
    }
    return t;
}

void rbh_plat_thread_join(rbh_thread *t)
{
    if (!t) return;
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle);
    rbh_free(t);
}

rbh_sem *rbh_plat_sem_create(void)
{
    rbh_sem *s = (rbh_sem *)rbh_alloc(sizeof *s);
    if (!s) return NULL;
    s->handle = CreateSemaphoreW(NULL, 0, 0x7fffffff, NULL);
    if (!s->handle) {
        rbh_free(s);
        return NULL;
    }
    return s;
}

void rbh_plat_sem_destroy(rbh_sem *s)
{
    if (!s) return;
    CloseHandle(s->handle);
    rbh_free(s);
}

void rbh_plat_sem_post(rbh_sem *s)
{
    ReleaseSemaphore(s->handle, 1, NULL);
}

int rbh_plat_sem_wait(rbh_sem *s, unsigned timeout_ms)
{
    return WaitForSingleObject(s->handle, timeout_ms) == WAIT_OBJECT_0;
}

rbh_mutex *rbh_plat_mutex_create(void)
{
    rbh_mutex *m = (rbh_mutex *)rbh_alloc(sizeof *m);
    if (m) InitializeSRWLock(&m->lock);
    return m;
}

void rbh_plat_mutex_destroy(rbh_mutex *m)
{
    rbh_free(m);
}

void rbh_plat_mutex_lock(rbh_mutex *m)
{
    rbh_audit_note(RBH_AUDIT_LOCK);
    AcquireSRWLockExclusive(&m->lock);
}

void rbh_plat_mutex_unlock(rbh_mutex *m)
{
    ReleaseSRWLockExclusive(&m->lock);
}

uint64_t rbh_plat_time_ns(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    /* whole seconds apart from the remainder: no overflow after days */
    const uint64_t seconds = (uint64_t)now.QuadPart / (uint64_t)freq.QuadPart;
    const uint64_t rest = (uint64_t)now.QuadPart % (uint64_t)freq.QuadPart;
    return seconds * 1000000000ull + rest * 1000000000ull / (uint64_t)freq.QuadPart;
}

void rbh_plat_sleep_ms(unsigned ms)
{
    Sleep(ms);
}

/* The most logical processors the render thread's choice looks at. */
#define RBH_MAX_CPUS 256

/* Keeps the calling thread to the fastest class of cores on a hybrid CPU
 * (cores.h), as a soft preference the scheduler honours (the thread's
 * "selected CPU sets", not a hard affinity), then reads it back into `out`.
 * No allocation: this runs on the render thread, which the audit watches. */
static void keep_to_fast_cores(rbh_priority *out)
{
    unsigned char info[RBH_MAX_CPUS * sizeof(SYSTEM_CPU_SET_INFORMATION)];
    unsigned char classes[RBH_MAX_CPUS];
    ULONG ids[RBH_MAX_CPUS], length = 0;
    int picked[RBH_MAX_CPUS], n = 0;
    if (!GetSystemCpuSetInformation((PSYSTEM_CPU_SET_INFORMATION)info, (ULONG)sizeof info, &length,
                                    GetCurrentProcess(), 0))
        return;
    for (ULONG at = 0; at < length && n < RBH_MAX_CPUS;) {
        const SYSTEM_CPU_SET_INFORMATION *e = (const SYSTEM_CPU_SET_INFORMATION *)(info + at);
        if (e->Size == 0) break;
        if (e->Type == CpuSetInformation) {
            ids[n] = e->CpuSet.Id;
            classes[n] = e->CpuSet.EfficiencyClass;
            n++;
        }
        at += e->Size;
    }
    out->all_cores = n;
    const int fast = rbh_pick_fast_cores(classes, n, picked);
    if (fast == 0) return;                          /* all alike: no preference */
    ULONG chosen[RBH_MAX_CPUS];
    for (int i = 0; i < fast; i++) chosen[i] = ids[picked[i]];
    if (!SetThreadSelectedCpuSets(GetCurrentThread(), chosen, (ULONG)fast)) return;
    ULONG back = 0;
    if (GetThreadSelectedCpuSets(GetCurrentThread(), chosen, RBH_MAX_CPUS, &back)) out->fast_cores = (int)back;
}

void rbh_plat_render_enter(rbh_priority *out)
{
    memset(out, 0, sizeof *out);
    out->applied = 1;
    /* a 1 ms timer, so the semaphore's timeouts are honest */
    if (timeBeginPeriod(1) == TIMERR_NOERROR) t_timer_raised = 1;
    DWORD index = 0;
    t_mmcss = AvSetMmThreadCharacteristicsW(L"Pro Audio", &index);
    if (t_mmcss) {
        out->boosted = 1;
        out->task_index = (unsigned)index;
        snprintf(out->policy, sizeof out->policy, "MMCSS Pro Audio");
    } else {
        /* no MMCSS (a service that's off, a restricted account): the
         * highest ordinary priority, and the policy says it isn't MMCSS */
        if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
            snprintf(out->policy, sizeof out->policy, "MMCSS unavailable (error %lu): thread priority highest",
                     (unsigned long)GetLastError());
        else
            snprintf(out->policy, sizeof out->policy, "no boost (MMCSS and SetThreadPriority failed)");
    }
    out->os_priority = GetThreadPriority(GetCurrentThread());
    keep_to_fast_cores(out);
    if (out->fast_cores > 0) {
        const size_t used = strlen(out->policy);
        snprintf(out->policy + used, sizeof out->policy - used, ", %d fast cores of %d", out->fast_cores,
                 out->all_cores);
    }
}

void rbh_plat_render_leave(void)
{
    if (t_mmcss) {
        AvRevertMmThreadCharacteristics(t_mmcss);
        t_mmcss = NULL;
    }
    if (t_timer_raised) {
        timeEndPeriod(1);
        t_timer_raised = 0;
    }
}

void rbh_plat_process_memory(size_t *now_bytes, size_t *peak_bytes)
{
    PROCESS_MEMORY_COUNTERS_EX counters;
    memset(&counters, 0, sizeof counters);
    counters.cb = sizeof counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&counters, sizeof counters)) {
        *now_bytes = (size_t)counters.PrivateUsage;
        *peak_bytes = (size_t)counters.PeakWorkingSetSize;
    } else {
        *now_bytes = 0;
        *peak_bytes = 0;
    }
}
