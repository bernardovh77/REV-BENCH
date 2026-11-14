/*
 * platform.h - what the host needs from the operating system (stage 123).
 *
 * The only place in the host that knows which system it runs on: threads, a
 * semaphore, a mutex, the clock, the render thread's priority and the
 * process's memory. platform_win.c (Windows, MMCSS) and platform_posix.c
 * (Linux, and macOS until stage 126 gives each its scheduling class) provide
 * it. Everything else in host/ is plain C11.
 */
#ifndef RBH_PLATFORM_H
#define RBH_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

typedef struct rbh_thread rbh_thread;
typedef struct rbh_sem    rbh_sem;
typedef struct rbh_mutex  rbh_mutex;
typedef void (*rbh_thread_fn)(void *arg);

/* Starts a thread running fn(arg): NULL if it couldn't. Join it to free it. */
rbh_thread *rbh_plat_thread_start(rbh_thread_fn fn, void *arg);
void        rbh_plat_thread_join(rbh_thread *t);

/* A counting semaphore. wait(ms) returns 1 if it took a count, 0 at the
 * timeout. The render thread waits on its own semaphore and nothing else. */
rbh_sem *rbh_plat_sem_create(void);
void     rbh_plat_sem_destroy(rbh_sem *s);
void     rbh_plat_sem_post(rbh_sem *s);
int      rbh_plat_sem_wait(rbh_sem *s, unsigned timeout_ms);

/* A mutex, for the control side only: locking one on the render thread is
 * reported by the audit (audit.h). */
rbh_mutex *rbh_plat_mutex_create(void);
void       rbh_plat_mutex_destroy(rbh_mutex *m);
void       rbh_plat_mutex_lock(rbh_mutex *m);
void       rbh_plat_mutex_unlock(rbh_mutex *m);

/* A monotonic clock, ns; and a sleep. */
uint64_t rbh_plat_time_ns(void);
void     rbh_plat_sleep_ms(unsigned ms);

/* What the system gave the render thread. */
typedef struct rbh_priority {
    int      applied;        /* rbh_plat_render_enter ran                     */
    int      boosted;        /* the system put it in a higher scheduling class */
    int      os_priority;    /* the thread's priority as the system reports it */
    unsigned task_index;
    char     policy[64];
    int      fast_cores;     /* the processors it keeps to (cores.h), read back */
    int      all_cores;
} rbh_priority;

/* Called by the render thread itself when it starts, and when it ends. */
void rbh_plat_render_enter(rbh_priority *out);
void rbh_plat_render_leave(void);

/* The process's memory in bytes: private (committed) now and the peak
 * working set (0 when unknown). */
void rbh_plat_process_memory(size_t *now_bytes, size_t *peak_bytes);

#endif /* RBH_PLATFORM_H */
