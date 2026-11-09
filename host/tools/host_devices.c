/*
 * host_devices.c - rb_host_devices (stage 123): lists the playback devices of a
 * backend, and the rate the host would render at on each. It opens nothing
 * and plays nothing.
 *
 *   rb_host_devices [--backend null|default]
 */
#include <stdio.h>
#include <string.h>

#include "rb_host.h"

int main(int argc, char **argv)
{
    int backend = RB_HOST_BACKEND_DEFAULT;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--backend") && i + 1 < argc) {
            ++i;
            if (!strcmp(argv[i], "null")) backend = RB_HOST_BACKEND_NULL;
            else if (!strcmp(argv[i], "default")) backend = RB_HOST_BACKEND_DEFAULT;
            else {
                fprintf(stderr, "rb_host_devices: --backend is null or default\n");
                return 2;
            }
        } else {
            fprintf(stderr, "usage: rb_host_devices [--backend null|default]\n");
            return 2;
        }
    }
    rb_host_device_info devices[32];
    const int n = rb_host_list_devices(backend, devices, 32);
    if (n < 0) {
        fprintf(stderr, "rb_host_devices: the audio backend could not be started (%d)\n", n);
        return 1;
    }
    printf("%d playback device%s\n", n, n == 1 ? "" : "s");
    for (int i = 0; i < n && i < 32; i++) {
        const int32_t rate = devices[i].native_rate;
        printf("  %2d%s %s: native rate %s, the host renders at %d Hz\n", devices[i].index,
               devices[i].is_default ? "*" : " ", devices[i].name, rate ? "known" : "any",
               rb_host_pick_rate(&rate, 1));
        if (rate) printf("      (native %d Hz)\n", (int)rate);
    }
    return 0;
}
