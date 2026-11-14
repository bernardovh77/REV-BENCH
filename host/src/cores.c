/*
 * cores.c - the render thread's choice of cores: see cores.h.
 */
#include "cores.h"

#include <stddef.h>

int rbh_pick_fast_cores(const unsigned char *classes, int count, int *picked)
{
    if (!classes || count <= 0) return 0;
    unsigned char top = classes[0], lowest = classes[0];
    for (int i = 1; i < count; i++) {
        if (classes[i] > top) top = classes[i];
        if (classes[i] < lowest) lowest = classes[i];
    }
    if (top == lowest) return 0;   /* every core alike: keep to none */
    int n = 0;
    for (int i = 0; i < count; i++)
        if (classes[i] == top) picked[n++] = i;
    return n;
}
