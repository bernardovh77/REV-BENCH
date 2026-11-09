/*
 * platform_posix.c - the host's POSIX layer: Linux, and macOS until stage 126
 * (stage 123).
 *
 * The render thread keeps the normal scheduling class here: enough for the
 * null device in CI. macOS's time-constraint policy and audio workgroup and
 * Linux's RTKit come with stage 126; until then the priority the diagnostics
 * report is "normal", honestly.
 */
#define _POSIX_C_SOURCE 200809L
#include "platform.h"

#include "audit.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Where the clock for condition variables can be CLOCK_MONOTONIC. */
#if defined(CLOCK_MONOTONIC) && !defined(__APPLE__)
#define RBH_COND_MONOTONIC 1
#else
#define RBH_COND_MONOTONIC 0
#endif

struct rbh_thread {
    pthread_t     handle;
    rbh_thread_fn fn;
    void         *arg;
};

/* A semaphore from a mutex and a condition variable: portable (macOS has no
 * unnamed POSIX semaphores) and it waits with a timeout. */
struct rbh_sem {
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    unsigned        count;
};

struct rbh_mutex {
    pthread_mutex_t lock;
};

static void *thread_main(void *p)
{
    rbh_thread *t = (rbh_thread *)p;
    t->fn(t->arg);
    return NULL;
}

rbh_thread *rbh_plat_thread_start(rbh_thread_fn fn, void *arg)
{
    rbh_thread *t = (rbh_thread *)rbh_alloc(sizeof *t);
    if (!t) return NULL;
    t->fn = fn;
    t->arg = arg;
    if (pthread_create(&t->handle, NULL, thread_main, t) != 0) {
        rbh_free(t);
        return NULL;
    }
    return t;
}

void rbh_plat_thread_join(rbh_thread *t)
{
    if (!t) return;
    pthread_join(t->handle, NULL);
    rbh_free(t);
}

rbh_sem *rbh_plat_sem_create(void)
{
    rbh_sem *s = (rbh_sem *)rbh_alloc(sizeof *s);
    if (!s) return NULL;
    pthread_mutex_init(&s->lock, NULL);
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
#if RBH_COND_MONOTONIC
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#endif
    pthread_cond_init(&s->cond, &attr);
    pthread_condattr_destroy(&attr);
    return s;
}

void rbh_plat_sem_destroy(rbh_sem *s)
{
    if (!s) return;
    pthread_cond_destroy(&s->cond);
    pthread_mutex_destroy(&s->lock);
    rbh_free(s);
}

void rbh_plat_sem_post(rbh_sem *s)
{
    pthread_mutex_lock(&s->lock);
    s->count++;
    pthread_mutex_unlock(&s->lock);
    pthread_cond_signal(&s->cond);
}

int rbh_plat_sem_wait(rbh_sem *s, unsigned timeout_ms)
{
    struct timespec until;
#if RBH_COND_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &until);
#else
    clock_gettime(CLOCK_REALTIME, &until);
#endif
    until.tv_sec += timeout_ms / 1000;
    until.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (until.tv_nsec >= 1000000000L) {
        until.tv_sec++;
        until.tv_nsec -= 1000000000L;
    }
    int got = 0;
    pthread_mutex_lock(&s->lock);
    while (s->count == 0) {
        if (pthread_cond_timedwait(&s->cond, &s->lock, &until) == ETIMEDOUT) break;
    }
    if (s->count > 0) {
        s->count--;
        got = 1;
    }
    pthread_mutex_unlock(&s->lock);
    return got;
}

rbh_mutex *rbh_plat_mutex_create(void)
{
    rbh_mutex *m = (rbh_mutex *)rbh_alloc(sizeof *m);
    if (m) pthread_mutex_init(&m->lock, NULL);
    return m;
}

void rbh_plat_mutex_destroy(rbh_mutex *m)
{
    if (!m) return;
    pthread_mutex_destroy(&m->lock);
    rbh_free(m);
}

void rbh_plat_mutex_lock(rbh_mutex *m)
{
    rbh_audit_note(RBH_AUDIT_LOCK);
    pthread_mutex_lock(&m->lock);
}

void rbh_plat_mutex_unlock(rbh_mutex *m)
{
    pthread_mutex_unlock(&m->lock);
}

uint64_t rbh_plat_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void rbh_plat_sleep_ms(unsigned ms)
{
    struct timespec ts = { (time_t)(ms / 1000), (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

void rbh_plat_render_enter(rbh_priority *out)
{
    memset(out, 0, sizeof *out);
    out->applied = 1;      /* nothing to ask for yet: the normal class is the policy */
    out->boosted = 0;
    out->os_priority = 0;
    snprintf(out->policy, sizeof out->policy, "normal (no boost until stage 126)");
}

void rbh_plat_render_leave(void)
{
}

void rbh_plat_process_memory(size_t *now_bytes, size_t *peak_bytes)
{
    *now_bytes = 0;
    *peak_bytes = 0;
    FILE *f = fopen("/proc/self/statm", "r");
    if (f) {
        unsigned long size = 0, resident = 0;
        if (fscanf(f, "%lu %lu", &size, &resident) == 2) *now_bytes = (size_t)resident * (size_t)sysconf(_SC_PAGESIZE);
        fclose(f);
    }
    f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof line, f)) {
            unsigned long kb;
            if (sscanf(line, "VmHWM: %lu kB", &kb) == 1) *peak_bytes = (size_t)kb * 1024u;
        }
        fclose(f);
    }
}
