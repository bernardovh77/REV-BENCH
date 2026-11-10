/*
 * host_stress.c - rb_host_stress (stage 123): a scripted drive on the real-time
 * host for minutes, reporting the deadline histogram, the ring's fill, the
 * underruns and the process's memory. It is the tool for the real-device run
 * (stage 123's exit evidence: 15 minutes, zero underruns at the default
 * latency, bounded memory), and runs on the null device in CI.
 *
 *   rb_host_stress [--backend null|default] [--minutes M | --seconds S]
 *                  [--preset KEY] [--presets KEY,KEY...]
 *                  [--quality tier|low|medium|high|ultra|balanced] [--device N]
 *                  [--rate 44100|48000] [--target-ms N] [--period-ms N]
 *                  [--swap-every S] [--csv FILE]
 *
 * The drive repeats a 20-second cycle through the control API only, as the app
 * will: the key and the starter, an idle, a blip, a full throttle into the
 * limiter, the foot off. Every --swap-every seconds (60 by default; 0 turns
 * it off) it prepares the next preset (of --presets, or all) on a worker
 * thread and swaps it in, and starts the drive over on the new engine.
 * `--quality tier` runs each preset at its declared real-time default
 * (rb_preset_quality, stage 118). Once a second it prints the
 * engine's speed (from the snapshots), the ring's fill, the underruns so far
 * and the process's memory.
 *
 * Exit code: 0 if there was no underrun, no forbidden call on the render
 * thread and the memory grew by less than 20 MB after the first 30 s; 1
 * otherwise. Blocks slower than their duration are reported (the deadline
 * histogram) but the ring is there to absorb them: only an underrun is heard; 2 for a bad
 * command line. The default backend plays on the real audio device: run it
 * only when meant to.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rb_host.h"

#define CYCLE_S 20.0
#define QUALITY_TIER (-2)   /* each preset at rb_preset_quality's default */
#define MAX_PRESETS 16

/* The quality to build `preset` at. */
static int quality_for(int quality, int preset)
{
    return quality == QUALITY_TIER ? rb_preset_quality(preset) : quality;
}

/* Parses "KEY,KEY,..." into preset numbers; returns how many, or -1. */
static int parse_presets(const char *list, int *out)
{
    char buf[256];
    int n = 0;
    if (strlen(list) >= sizeof buf) return -1;
    strcpy(buf, list);
    for (char *key = strtok(buf, ","); key; key = strtok(NULL, ",")) {
        const int p = rb_preset_find(key);
        if (p < 0 || n >= MAX_PRESETS) return -1;
        out[n++] = p;
    }
    return n > 0 ? n : -1;
}

static double seconds_since(uint64_t t0)
{
    return (double)(rb_host_time_ns() - t0) / 1e9;
}

/* The engine's rpm from the newest snapshot (the first instrument); keeps the last value. */
static double read_rpm(rb_host *h, double last)
{
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    rb_host_snapshot_info info;
    double rpm = last;
    while (rb_host_snapshot_read(h, buf, (int)sizeof buf, &info) > 0) {
        if (info.size >= sizeof(rb_snapshot_header) + sizeof(double)) memcpy(&rpm, buf + sizeof(rb_snapshot_header), sizeof rpm);
    }
    return rpm;
}

/* The driver's script at `t` seconds into a cycle: sends only what changes. */
typedef struct driver {
    double throttle;
    int    starter, started;
} driver;

static void drive(rb_host *h, driver *d, double t)
{
    double throttle = 0.0;
    if (t >= 6.0 && t < 6.3) throttle = 0.4;
    else if (t >= 8.5 && t < 10.1) throttle = 1.0;
    if (!d->started) {
        rb_host_set_param(h, RB_PARAM_HOLD, 0, 0, NULL);
        rb_host_set_param(h, RB_PARAM_IGNITION, 1, 0, NULL);
        rb_host_set_param(h, RB_PARAM_STARTER, 1, 0, NULL);
        d->starter = 1;
        d->started = 1;
    }
    if (d->starter && t > 3.0) {
        rb_host_set_param(h, RB_PARAM_STARTER, 0, 0, NULL);
        d->starter = 0;
    }
    if (throttle != d->throttle) {
        rb_host_set_param(h, RB_PARAM_THROTTLE, throttle, 0, NULL);
        d->throttle = throttle;
    }
}

int main(int argc, char **argv)
{
    int backend = RB_HOST_BACKEND_NULL, device = -1, rate = 48000, quality = RB_QUALITY_HIGH, target_ms = 20, period_ms = 10;
    double seconds = 15.0 * 60.0, swap_every = 60.0;
    const char *preset = "i4", *csv_path = NULL;
    int cycle[MAX_PRESETS], n_cycle = 0;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--backend") && v) {
            backend = !strcmp(v, "default") ? RB_HOST_BACKEND_DEFAULT : !strcmp(v, "null") ? RB_HOST_BACKEND_NULL : -1;
            i++;
        } else if (!strcmp(a, "--minutes") && v) { seconds = 60.0 * atof(v); i++; }
        else if (!strcmp(a, "--seconds") && v) { seconds = atof(v); i++; }
        else if (!strcmp(a, "--preset") && v) { preset = v; i++; }
        else if (!strcmp(a, "--presets") && v) {
            n_cycle = parse_presets(v, cycle);
            if (n_cycle < 0) {
                fprintf(stderr, "rb_host_stress: a bad preset list '%s'\n", v);
                return 2;
            }
            i++;
        }
        else if (!strcmp(a, "--quality") && v) {
            quality = !strcmp(v, "low") ? RB_QUALITY_LOW : !strcmp(v, "medium") ? RB_QUALITY_MEDIUM
                    : !strcmp(v, "high") ? RB_QUALITY_HIGH : !strcmp(v, "ultra") ? RB_QUALITY_ULTRA
                    : !strcmp(v, "balanced") ? RB_QUALITY_BALANCED : !strcmp(v, "tier") ? QUALITY_TIER : -1;
            i++;
        } else if (!strcmp(a, "--device") && v) { device = atoi(v); i++; }
        else if (!strcmp(a, "--rate") && v) { rate = atoi(v); i++; }
        else if (!strcmp(a, "--target-ms") && v) { target_ms = atoi(v); i++; }
        else if (!strcmp(a, "--period-ms") && v) { period_ms = atoi(v); i++; }
        else if (!strcmp(a, "--swap-every") && v) { swap_every = atof(v); i++; }
        else if (!strcmp(a, "--csv") && v) { csv_path = v; i++; }
        else {
            fprintf(stderr, "rb_host_stress: unknown or incomplete option '%s' (see the top of host/tools/host_stress.c)\n", a);
            return 2;
        }
    }
    if (backend < 0 || (quality < 0 && quality != QUALITY_TIER) || !(seconds > 0.0) || rb_preset_find(preset) < 0) {
        fprintf(stderr, "rb_host_stress: a bad backend, quality, duration or preset\n");
        return 2;
    }

    rb_host_config config;
    rb_host_config_default(&config);
    config.backend = backend;
    config.device = device;
    config.sample_rate = rate;
    config.target_fill_ms = target_ms;
    config.period_ms = period_ms;
    config.initial.preset = rb_preset_find(preset);
    config.initial.quality = quality_for(quality, config.initial.preset);
    if (n_cycle == 0)
        for (int p = 0; p < rb_preset_count() && p < MAX_PRESETS; p++) cycle[n_cycle++] = p;
    rb_host *h = NULL;
    int rc = rb_host_create(&config, &h);
    if (rc != RB_HOST_OK) {
        fprintf(stderr, "rb_host_stress: the host could not be made (%d)\n", rc);
        return 1;
    }
    rc = rb_host_start(h);
    if (rc != RB_HOST_OK) {
        fprintf(stderr, "rb_host_stress: the host could not start (%d: the audio device?)\n", rc);
        rb_host_destroy(h);
        return 1;
    }
    FILE *csv = csv_path ? fopen(csv_path, "w") : NULL;
    if (csv) fprintf(csv, "t_s,rpm,ring_fill,underruns,deadline_missed,render_ms_max,process_mb\n");

    rb_host_stats s0, s;
    rb_host_get_stats(h, &s0);
    printf("rb_host_stress: %s backend, %.0f Hz, %s, %.0f s%s; render thread: %s\n",
           backend == RB_HOST_BACKEND_NULL ? "null" : "DEFAULT (real device)", rb_host_sample_rate(h), preset, seconds,
           swap_every > 0.0 ? "" : ", no swaps", s0.priority.policy);
    printf("%7s %6s %5s %9s %7s %9s %8s\n", "t (s)", "rpm", "fill", "underruns", "missed", "max (ms)", "MB");

    const uint64_t t0 = rb_host_time_ns();
    driver drv = { 0.0, 0, 0 };
    double cycle_start = 0.0, next_report = 1.0, next_swap = swap_every, rpm = 0.0;
    uint64_t swap_prepare = 0, swap_request = 0;
    int at = 0, swaps_done = 0, memory_baseline_set = 0;
    for (int k = 0; k < n_cycle; k++)
        if (cycle[k] == config.initial.preset) at = k;
    double memory_baseline = 0.0, memory_worst = 0.0;
    for (double t = 0.0; t < seconds; t = seconds_since(t0)) {
        drive(h, &drv, t - cycle_start);
        if (t - cycle_start >= CYCLE_S) {      /* the next cycle: idle on, the pedal off, from the top of the script */
            cycle_start += CYCLE_S;
            drv.starter = 0;
            drv.throttle = 0.0;
            rb_host_set_param(h, RB_PARAM_THROTTLE, 0.0, 0, NULL);
        }
        rb_host_ack acks[16];
        const int n = rb_host_poll(h, acks, 16);
        for (int k = 0; k < n; k++)
            if (acks[k].kind == RB_HOST_CMD_SWAP && acks[k].request_id == swap_request) {
                swap_request = 0;
                swaps_done += acks[k].outcome == RB_HOST_ACK_OK;
                drv.started = 0;               /* the new engine is at rest: the key again */
                drv.starter = 0;
                drv.throttle = 0.0;
                cycle_start = t;
            }
        if (swap_every > 0.0 && t >= next_swap && !swap_prepare && !swap_request) {
            rb_host_build b;
            rb_host_build_default(&b);
            at = (at + 1) % n_cycle;
            b.preset = cycle[at];
            b.quality = quality_for(quality, cycle[at]);
            if (rb_host_prepare_begin(h, &b, &swap_prepare) != RB_HOST_OK) swap_prepare = 0;
            next_swap = t + swap_every;
        }
        if (swap_prepare) {
            const int state = rb_host_prepare_wait(h, swap_prepare, 0);
            if (state == RB_HOST_PREPARE_READY) {
                if (rb_host_swap(h, swap_prepare, 0, &swap_request) != RB_HOST_OK) swap_request = 0;
                swap_prepare = 0;
            } else if (state == RB_HOST_PREPARE_FAILED) {
                rb_host_prepare_discard(h, swap_prepare);
                swap_prepare = 0;
            }
        }
        rpm = read_rpm(h, rpm);
        if (t >= next_report) {
            next_report += 1.0;
            rb_host_get_stats(h, &s);
            const double mb = (double)s.process_bytes / (1024.0 * 1024.0);
            if (t >= 30.0 && !memory_baseline_set) {
                memory_baseline = mb;
                memory_baseline_set = 1;
            }
            if (memory_baseline_set && mb - memory_baseline > memory_worst) memory_worst = mb - memory_baseline;
            const double max_ms = (double)s.render_ns_max / 1e6;
            if (((long)t) % 10 == 0 || s.underruns > s0.underruns)
                printf("%7.0f %6.0f %5u %9llu %7llu %9.2f %8.1f\n", t, rpm, s.ring_fill_last, (unsigned long long)s.underruns,
                       (unsigned long long)s.deadline_missed, max_ms, mb);
            if (csv)
                fprintf(csv, "%.1f,%.0f,%u,%llu,%llu,%.3f,%.2f\n", t, rpm, s.ring_fill_last, (unsigned long long)s.underruns,
                        (unsigned long long)s.deadline_missed, max_ms, mb);
        }
        const uint64_t nap = rb_host_time_ns();
        while (rb_host_time_ns() - nap < 4000000ull) {
        }
    }
    rb_host_get_stats(h, &s);
    if (csv) fclose(csv);
    rb_host_stop(h);

    printf("\nblocks rendered %llu, one every %.2f ms of audio; swaps %d\n", (unsigned long long)s.blocks,
           128000.0 / rb_host_sample_rate(h), swaps_done);
    printf("render time per block: mean %.3f ms, max %.3f ms (the deadline is %.2f ms)\n",
           s.blocks ? (double)s.render_ns_total / (double)s.blocks / 1e6 : 0.0, (double)s.render_ns_max / 1e6,
           128000.0 / rb_host_sample_rate(h));
    printf("deadline histogram, render time over the block's duration (share of blocks):\n ");
    for (int i = 0; i < 10; i++) printf(" %d-%d%%: %.4f", i * 10, i * 10 + 10, s.blocks ? (double)s.deadline[i] / (double)s.blocks : 0.0);
    printf("\n  100-200%%: %llu  200%%+: %llu  (missed: %llu)\n", (unsigned long long)s.deadline[10],
           (unsigned long long)s.deadline[11], (unsigned long long)s.deadline_missed);
    printf("ring: capacity %u, target %u, fill min %u mean %.0f max %u frames\n", s.ring_capacity, s.ring_target, s.ring_fill_min,
           s.ring_fill_mean, s.ring_fill_max);
    printf("device: %llu callbacks, %llu frames; underruns %llu (%llu frames of silence)\n", (unsigned long long)s.device_callbacks,
           (unsigned long long)s.frames_played, (unsigned long long)s.underruns, (unsigned long long)s.underrun_frames);
    printf("memory: %.1f MB at the end, peak working set %.1f MB, growth after 30 s: %.2f MB\n",
           (double)s.process_bytes / 1048576.0, (double)s.process_peak_bytes / 1048576.0, memory_worst);
    printf("render thread priority: %s (os priority %d)\n", s.priority.policy, s.priority.os_priority);
    printf("render thread's forbidden calls: allocations %llu, locks %llu, logs %llu\n", (unsigned long long)s.audit_alloc,
           (unsigned long long)s.audit_lock, (unsigned long long)s.audit_log);
    rb_host_destroy(h);
    const int bad = s.underruns > 0 || memory_worst > 20.0 || s.audit_alloc + s.audit_lock + s.audit_log > 0;
    printf("%s\n", bad ? "FAILED: underruns, memory growth or a forbidden call" : "ok");
    return bad ? 1 : 0;
}
