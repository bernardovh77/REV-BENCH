/*
 * audit.c - the render thread's rules, checked: see audit.h.
 */
#include "audit.h"

#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

/* Set by the render thread for itself. */
static _Thread_local int t_render;

static atomic_uint_least64_t counts[RBH_AUDIT_KINDS];
static atomic_int_least64_t  live;

void rbh_audit_enter_render(void)
{
    t_render = 1;
}

void rbh_audit_leave_render(void)
{
    t_render = 0;
}

int rbh_audit_in_render(void)
{
    return t_render;
}

void rbh_audit_note(int kind)
{
    if (t_render && kind >= 0 && kind < RBH_AUDIT_KINDS)
        atomic_fetch_add_explicit(&counts[kind], 1, memory_order_relaxed);
}

uint64_t rbh_audit_count(int kind)
{
    if (kind < 0 || kind >= RBH_AUDIT_KINDS) return 0;
    return atomic_load_explicit(&counts[kind], memory_order_relaxed);
}

void rbh_audit_reset(void)
{
    for (int i = 0; i < RBH_AUDIT_KINDS; i++) atomic_store_explicit(&counts[i], 0, memory_order_relaxed);
}

void *rbh_alloc(size_t bytes)
{
    rbh_audit_note(RBH_AUDIT_ALLOC);
    void *p = calloc(1, bytes ? bytes : 1);
    if (p) atomic_fetch_add_explicit(&live, 1, memory_order_relaxed);
    return p;
}

void rbh_free(void *p)
{
    if (!p) return;
    rbh_audit_note(RBH_AUDIT_ALLOC);
    atomic_fetch_sub_explicit(&live, 1, memory_order_relaxed);
    free(p);
}

void rbh_log(const char *format, ...)
{
    rbh_audit_note(RBH_AUDIT_LOG);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

int64_t rbh_alloc_live(void)
{
    return atomic_load_explicit(&live, memory_order_relaxed);
}
