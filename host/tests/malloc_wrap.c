/*
 * malloc_wrap.c - the C library's allocator, watched (stage 123). The test
 * binary is linked with `-Wl,--wrap=malloc` (and calloc, realloc, free), so
 * every call the host or the core makes goes through these functions, which
 * count the ones made on the render thread. The host's own audit (audit.h)
 * sees only what goes through its wrappers; this sees a malloc anywhere,
 * the core's included. GNU ld only (Linux, MinGW): MSVC and macOS builds rely
 * on the audit alone.
 */
#ifdef RB_HOST_MALLOC_WRAP

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "../src/audit.h"

void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void *__real_realloc(void *p, size_t bytes);
void  __real_free(void *p);

static atomic_uint_least64_t render_calls;

static void note(void)
{
    if (rbh_audit_in_render()) atomic_fetch_add_explicit(&render_calls, 1, memory_order_relaxed);
}

void *__wrap_malloc(size_t bytes)
{
    note();
    return __real_malloc(bytes);
}

void *__wrap_calloc(size_t count, size_t bytes)
{
    note();
    return __real_calloc(count, bytes);
}

void *__wrap_realloc(void *p, size_t bytes)
{
    note();
    return __real_realloc(p, bytes);
}

void __wrap_free(void *p)
{
    note();
    __real_free(p);
}

uint64_t rbh_wrap_render_calls(void)
{
    return atomic_load_explicit(&render_calls, memory_order_relaxed);
}

#endif /* RB_HOST_MALLOC_WRAP */
