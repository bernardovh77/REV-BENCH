/*
 * audit.h - the render thread's rules, checked (stage 123).
 *
 * The render thread never allocates, locks or logs. Trust isn't enough: all
 * the host's allocation, locking and logging go through the functions here
 * and in platform.h, which count a call made on a thread that has marked
 * itself the render thread. The tests read the counts (rb_host_stats'
 * audit_alloc, audit_lock, audit_log) after the render thread has run, and
 * the test build also wraps the C library's malloc family where the linker
 * allows (host/tests/malloc_wrap.c).
 */
#ifndef RBH_AUDIT_H
#define RBH_AUDIT_H

#include <stddef.h>
#include <stdint.h>

/* The calling thread is the render thread, from now until leave. */
void rbh_audit_enter_render(void);
void rbh_audit_leave_render(void);
int  rbh_audit_in_render(void);

/* Which rule was broken. */
enum { RBH_AUDIT_ALLOC = 0, RBH_AUDIT_LOCK, RBH_AUDIT_LOG, RBH_AUDIT_KINDS };
/* Counts a broken rule if the calling thread is the render thread. */
void     rbh_audit_note(int kind);
uint64_t rbh_audit_count(int kind);
/* Forgets the counts (for a test that broke a rule on purpose). */
void     rbh_audit_reset(void);

/* The host's allocation, locking and logging. */
void *rbh_alloc(size_t bytes);              /* zeroed; NULL if out of memory  */
void  rbh_free(void *p);
void  rbh_log(const char *format, ...);     /* to stderr: control side only   */

/* Allocations made and freed by the host so far (for leak tests). */
int64_t rbh_alloc_live(void);

#endif /* RBH_AUDIT_H */
