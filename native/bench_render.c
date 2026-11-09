/*
 * bench_render.c - rb_bench_render: how fast the app's whole render path runs
 * (stage 118).
 *
 *   rb_bench_render [PRESET...] [--quality LIST] [--drive LIST] [--runs N]
 *                   [--rate HZ] [--scene NAME] [--affinity MASK] [--ftz]
 *                   [--csv FILE] [--wav-dir DIR] [--figures]
 *
 * rb_bench (stage 13) times the engine's steps alone. The app runs more than
 * that: rb_render_stereo with the noise, the listener, the stereo scene, the
 * anti-alias decimators, the DC blockers and the soft limiter, in blocks of
 * 128 frames. This program times exactly that, through rb.h alone (as the
 * app's host will, and as rb_cli sound does: it includes no header from
 * core/src), for each preset, quality and drive, with the variation on.
 *
 * The drives are rb_cli sound's, a driver's script of parameters set between
 * blocks. Each has a warm-up that isn't timed, then a window that is:
 *   idle  the key, the starter, 3 s, then 3 s idling;
 *   hold  held at 3000 rpm wide open 1 s, then 3 s on a dyno there;
 *   rev   the key, the starter, 3 s idling, then 7 s: a blip, floored into
 *         the rev limiter, the foot off and back to idle.
 * Each cell is rendered N times from scratch (the render is deterministic,
 * so every run does the same work) and the best is kept. The cost is the
 * window's seconds of computing per second of sound ("x real time" is its
 * inverse), and the slowest block matters too: a block that takes longer
 * than its 128 / rate seconds is an underrun unless the ring has slack.
 *
 * Every cell also prints a hash of all the floats it rendered (warm-up
 * included): the proof that a change to the core computes the same thing,
 * for the whole table at once (tools/bench_render.py compares two builds').
 *
 * --ftz sets the processor's flush-to-zero and denormals-are-zero modes (x86
 * only), to measure whether subnormal numbers cost anything and whether they
 * change the output (the hash tells). --affinity pins the process to the
 * logical processors in a hex MASK, since on a hybrid CPU the render thread
 * may land on a performance or an efficiency core.
 *
 * --figures instead measures what the tier changes in the engine itself:
 * for each preset and quality, the engine held wide open at several speeds
 * (variation off, so the figures are exact: the mean brake torque, power, the
 * manifold pressure and the air flow as a volumetric efficiency over a few
 * seconds) and idling after the key (variation on: the idle speed's mean and
 * its standard deviation). tools/tier_compare.py puts them side by side.
 *
 * Output: a CSV row per cell on stdout (or into --csv), and a readable line
 * per cell on stderr. --wav-dir writes each cell's rendered window as a
 * float WAV (stage 118's listening comparisons), named
 * PRESET-QUALITY-DRIVE.wav.
 */
#define _POSIX_C_SOURCE 200809L   /* clock_gettime, sched_setaffinity */
#define _GNU_SOURCE               /* cpu_set_t */

#include "rb.h"
#include "wav.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The clock, s: Windows' performance counter under MSVC and w64devkit, POSIX's
 * elsewhere; and the process's affinity, which is system-specific by nature,
 * so it stays in this file's one #ifdef. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double now(void)
{
    LARGE_INTEGER count, rate;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&rate);
    return (double)count.QuadPart / (double)rate.QuadPart;
}
static int pin_to(unsigned long long mask)
{
    return SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)mask) ? 0 : -1;
}
#else
#include <time.h>
static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}
#ifdef __wasi__   /* WebAssembly (make bench-render-wasm): one thread, no cores to choose */
static int pin_to(unsigned long long mask)
{
    (void)mask;
    return -1;
}
#else
#include <sched.h>
static int pin_to(unsigned long long mask)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int k = 0; k < 64; k++)
        if (mask >> k & 1ULL) CPU_SET(k, &set);
    return sched_setaffinity(0, sizeof set, &set);
}
#endif
#endif

/* Flush-to-zero and denormals-are-zero for the SSE unit (x86 and x86-64). */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h>
static int set_ftz(void)
{
    _mm_setcsr(_mm_getcsr() | 0x8040u);   /* FZ (bit 15) and DAZ (bit 6) */
    return 0;
}
#else
static int set_ftz(void)
{
    return -1;
}
#endif

/* Rendered in blocks of 128 frames, as the app's host will and rb_cli does. */
#define BLOCK_FRAMES 128

enum { DRIVE_IDLE, DRIVE_HOLD, DRIVE_REV, DRIVE_COUNT };
static const char *const DRIVES[DRIVE_COUNT] = { "idle", "hold", "rev" };
/* The qualities rb.h names, then variants the tier's search tried (stage 118):
 * physics at 2 or 3 times the sample rate with the pipes at their default
 * 48 kHz, which no named quality has (low and medium cut the pipes to 24 kHz). */
#define EXTRA_QUALITIES 2
#define QUALITY_COUNT (RB_QUALITY_COUNT + EXTRA_QUALITIES)
static const char *const QUALITIES[QUALITY_COUNT] = { "low", "medium", "high", "ultra", "os2", "os3" };

/* Sets cfg's oversample and pipe rate for quality q (an index of QUALITIES). */
static int configure(rb_config *cfg, int q)
{
    if (q < RB_QUALITY_COUNT) return rb_config_quality(cfg, q);
    cfg->oversample = q - RB_QUALITY_COUNT + 2;
    cfg->pipe_rate = 0.0;   /* the default, 48 kHz */
    return RB_OK;
}
static const char *const SCENES[RB_SCENE_COUNT] = { "asphalt", "grass", "wall", "garage",
                                                    "dyno", "iso", "passby", "cabin" };

/* Seconds of warm-up and of timed window for each drive. */
static const double WARM_S[DRIVE_COUNT] = { 3.0, 1.0, 3.0 };
static const double WINDOW_S[DRIVE_COUNT] = { 3.0, 3.0, 7.0 };

#define HOLD_RPM        3000.0
#define REV_BLIP_AT     3.0   /* s from the key, as rb_cli sound's --rev */
#define REV_BLIP        0.25
#define REV_BLIP_THROTTLE 0.4
#define REV_FLOOR_AT    5.5
#define REV_FLOOR       1.6

/* One cell's results. */
typedef struct cell_result {
    double window_s;    /* seconds of sound timed                              */
    double cpu_s;       /* seconds of computing for them: the best run's       */
    double worst_ms;    /* the slowest block of the best run, ms               */
    double p99_ms;      /* the 99th percentile of its blocks, ms               */
    double deadline_ms; /* a block's duration, ms                              */
    uint64_t hash;      /* of every float rendered, warm-up included           */
    double nonfinite;
    double rpm_end;     /* the engine's speed at the end of the window         */
} cell_result;

/* FNV-1a over the bytes of the floats, for comparing runs and builds. */
static uint64_t hash_floats(uint64_t h, const float *x, size_t n)
{
    const unsigned char *p = (const unsigned char *)x;
    for (size_t i = 0; i < n * sizeof(float); i++) h = (h ^ p[i]) * 1099511628211ULL;
    return h;
}

/* For qsort: doubles ascending. */
static int by_value(const void *a, const void *b)
{
    const double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* The driver's script at t seconds since the key (or the hold), for the
 * drives that have one. */
static void drive_script(rb_sim *sim, int drive, double t)
{
    if (drive == DRIVE_IDLE || drive == DRIVE_REV) {
        if (t > 3.0 || (rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH) > 0.75 * rb_get_engine(sim, RB_ENG_IDLE_RPM, 0)))
            rb_set_param(sim, RB_PARAM_STARTER, 0);
    }
    if (drive == DRIVE_REV)
        rb_set_param(sim, RB_PARAM_THROTTLE,
                     (t >= REV_BLIP_AT && t < REV_BLIP_AT + REV_BLIP) ? REV_BLIP_THROTTLE
                   : (t >= REV_FLOOR_AT && t < REV_FLOOR_AT + REV_FLOOR) ? 1.0 : 0.0);
}

/* Renders one cell once: warm-up, then the timed window. Fills r (hash,
 * timing). If wav is not NULL the window's frames are written to it. 0, or -1
 * if the sim can't be built. */
static int run_cell(int preset, int quality, int drive, double rate, int scene, cell_result *r,
                    wav_writer *wav, double *blocks_ms, long max_blocks)
{
    rb_config cfg;
    rb_config_default(&cfg);
    cfg.sample_rate = rate;
    if (configure(&cfg, quality) != RB_OK) return -1;
    rb_sim *sim = rb_create(&cfg);
    if (!sim) return -1;
    rb_load_preset(sim, preset);
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_VARIATION, 1.0);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    if (drive == DRIVE_HOLD) {
        rb_set_param(sim, RB_PARAM_HOLD_RPM, HOLD_RPM);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    } else {
        rb_set_param(sim, RB_PARAM_HOLD, 0);
        rb_set_param(sim, RB_PARAM_THROTTLE, 0.0);
        rb_set_param(sim, RB_PARAM_STARTER, 1);
    }
    rb_set_scene(sim, scene);

    static float buf[2 * BLOCK_FRAMES];
    uint64_t hash = 14695981039346656037ULL;
    const long warm = lround(WARM_S[drive] * rate / BLOCK_FRAMES);
    const long timed = lround(WINDOW_S[drive] * rate / BLOCK_FRAMES);
    if (timed > max_blocks) {
        rb_destroy(sim);
        return -1;
    }
    for (long k = 0; k < warm; k++) {
        drive_script(sim, drive, k * BLOCK_FRAMES / rate);
        rb_render_stereo(sim, buf, BLOCK_FRAMES);
        hash = hash_floats(hash, buf, 2 * BLOCK_FRAMES);
    }
    if (drive == DRIVE_HOLD) {   /* as rb_cli sound's --hold: a dyno takes the crank */
        if (rb_get_telemetry(sim, RB_TLM_TORQUE) > 0.0) rb_dyno_attach(sim, HOLD_RPM);
    } else {
        rb_set_param(sim, RB_PARAM_STARTER, 0);
        rb_set_param(sim, RB_PARAM_LOAD_TORQUE, 0.0);
    }
    const double t_start = warm * BLOCK_FRAMES / rate;
    const double nonfinite0 = rb_get_telemetry(sim, RB_TLM_NONFINITE);
    double total = 0.0;
    for (long k = 0; k < timed; k++) {
        drive_script(sim, drive, drive == DRIVE_HOLD ? 0.0 : t_start + k * BLOCK_FRAMES / rate);
        const double t0 = now();
        rb_render_stereo(sim, buf, BLOCK_FRAMES);
        const double dt = now() - t0;
        blocks_ms[k] = dt * 1e3;
        total += dt;
        hash = hash_floats(hash, buf, 2 * BLOCK_FRAMES);
        if (wav) wav_write(wav, buf, BLOCK_FRAMES);
    }
    r->window_s = timed * BLOCK_FRAMES / rate;
    r->cpu_s = total;
    r->deadline_ms = 1e3 * BLOCK_FRAMES / rate;
    r->hash = hash;
    r->nonfinite = rb_get_telemetry(sim, RB_TLM_NONFINITE) - nonfinite0;
    r->rpm_end = rb_get_telemetry(sim, RB_TLM_RPM);
    rb_destroy(sim);
    return 0;
}


/* The speeds --figures holds the engine at, rpm: from just above idle to
 * near the rev limit of the smallest engines. */
static const double FIGURE_RPM[] = { 1500.0, 3000.0, 4500.0, 6000.0 };
#define FIGURE_POINTS 4
#define FIGURE_SETTLE_S 1.5
#define FIGURE_WINDOW_S 4.0
#define AIR_DENSITY 1.184   /* kg/m^3, the reference for the volumetric efficiency */

/* One preset at one quality, held wide open at each FIGURE_RPM with the
 * variation off, then idling after the key with it on: prints a CSV row each. */
static int figures(int preset, int quality, double rate)
{
    rb_config cfg;
    rb_config_default(&cfg);
    cfg.sample_rate = rate;
    if (configure(&cfg, quality) != RB_OK) return -1;
    static float buf[2 * BLOCK_FRAMES];
    for (int k = 0; k < FIGURE_POINTS + 1; k++) {
        rb_sim *sim = rb_create(&cfg);
        if (!sim) return -1;
        rb_load_preset(sim, preset);
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        const int idle = k == FIGURE_POINTS;
        const double rpm = idle ? 0.0 : FIGURE_RPM[k];
        rb_set_param(sim, RB_PARAM_VARIATION, idle ? 1.0 : 0.0);
        if (idle) {
            rb_set_param(sim, RB_PARAM_HOLD, 0);
            rb_set_param(sim, RB_PARAM_THROTTLE, 0.0);
            rb_set_param(sim, RB_PARAM_STARTER, 1);
        } else {
            rb_set_param(sim, RB_PARAM_HOLD_RPM, rpm);
            rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
        }
        const long settle = lround((idle ? 5.0 : FIGURE_SETTLE_S) * rate / BLOCK_FRAMES);
        const long window = lround((idle ? 8.0 : FIGURE_WINDOW_S) * rate / BLOCK_FRAMES);
        for (long b = 0; b < settle; b++) {
            if (idle && rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH) > 0.75 * rb_get_engine(sim, RB_ENG_IDLE_RPM, 0))
                rb_set_param(sim, RB_PARAM_STARTER, 0);
            rb_render_block(sim, buf, BLOCK_FRAMES);
        }
        rb_set_param(sim, RB_PARAM_STARTER, 0);
        double torque = 0.0, power = 0.0, air = 0.0, map = 0.0, rpm_sum = 0.0, rpm_sq = 0.0;
        for (long b = 0; b < window; b++) {
            rb_render_block(sim, buf, BLOCK_FRAMES);
            torque += rb_get_telemetry(sim, RB_TLM_TORQUE);
            power += rb_get_telemetry(sim, RB_TLM_POWER);
            air += rb_get_telemetry(sim, RB_TLM_AIR_FLOW);
            map += rb_get_telemetry(sim, RB_TLM_MAP);
            const double r = rb_get_telemetry(sim, RB_TLM_RPM);
            rpm_sum += r;
            rpm_sq += r * r;
        }
        const double n = (double)window;
        const double mean_rpm = rpm_sum / n;
        const double displacement = rb_get_engine(sim, RB_ENG_DISPLACEMENT, 0);
        const double ve = air / n / (AIR_DENSITY * displacement * mean_rpm / 120.0);
        printf("%s,%s,%s,%.0f,%.4f,%.2f,%.5f,%.3f,%.3f,%.3f\n", rb_preset_key(preset), QUALITIES[quality],
               idle ? "idle" : "held", idle ? mean_rpm : rpm, torque / n, power / n, ve, map / n * 1e-3,
               mean_rpm, sqrt(fmax(0.0, rpm_sq / n - mean_rpm * mean_rpm)));
        rb_destroy(sim);
    }
    return 0;
}

/* Reads a comma-separated list of names (from table) into an array of
 * indices; the word "all" is every one. The count, or 0 if a name is wrong. */
static int names_from(const char *list, const char *const *table, int n, int *out)
{
    int count = 0;
    if (!strcmp(list, "all")) {
        for (int k = 0; k < n; k++) out[count++] = k;
        return count;
    }
    while (*list) {
        const char *end = strchr(list, ',');
        const size_t len = end ? (size_t)(end - list) : strlen(list);
        int found = -1;
        for (int k = 0; k < n; k++)
            if (strlen(table[k]) == len && !strncmp(list, table[k], len)) found = k;
        if (found < 0 || count == n) return 0;
        out[count++] = found;
        list += len + (end != NULL);
    }
    return count;
}

int main(int argc, char **argv)
{
    int presets[32], n_presets = 0;
    int qualities[QUALITY_COUNT] = { RB_QUALITY_LOW, RB_QUALITY_MEDIUM, RB_QUALITY_HIGH }, n_qualities = 3;
    int drives[DRIVE_COUNT] = { DRIVE_IDLE, DRIVE_HOLD, DRIVE_REV }, n_drives = DRIVE_COUNT;
    int runs = 3, scene = RB_SCENE_PASSBY, ftz = 0, want_figures = 0;
    double rate = 48000.0;
    unsigned long long mask = 0;
    const char *csv_path = NULL, *wav_dir = NULL;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--quality") && v) {
            n_qualities = names_from(argv[++i], QUALITIES, QUALITY_COUNT, qualities);
            if (!n_qualities) { fprintf(stderr, "rb_bench_render: bad --quality\n"); return 2; }
        } else if (!strcmp(a, "--drive") && v) {
            n_drives = names_from(argv[++i], DRIVES, DRIVE_COUNT, drives);
            if (!n_drives) { fprintf(stderr, "rb_bench_render: bad --drive\n"); return 2; }
        } else if (!strcmp(a, "--runs") && v) {
            runs = atoi(argv[++i]);
            if (runs < 1 || runs > 100) { fprintf(stderr, "rb_bench_render: bad --runs\n"); return 2; }
        } else if (!strcmp(a, "--rate") && v) {
            rate = atof(argv[++i]);
        } else if (!strcmp(a, "--scene") && v) {
            int s[RB_SCENE_COUNT];
            if (names_from(argv[++i], SCENES, RB_SCENE_COUNT, s) != 1) { fprintf(stderr, "rb_bench_render: bad --scene\n"); return 2; }
            scene = s[0];
        } else if (!strcmp(a, "--affinity") && v) {
            mask = strtoull(argv[++i], NULL, 16);
        } else if (!strcmp(a, "--ftz")) {
            ftz = 1;
        } else if (!strcmp(a, "--figures")) {
            want_figures = 1;
        } else if (!strcmp(a, "--csv") && v) {
            csv_path = argv[++i];
        } else if (!strcmp(a, "--wav-dir") && v) {
            wav_dir = argv[++i];
        } else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            printf("usage: rb_bench_render [PRESET...] [--quality low,medium,high,ultra,os2,os3|all] "
                   "[--drive idle,hold,rev|all] [--runs N] [--rate HZ] [--scene NAME]\n"
                   "                       [--affinity HEXMASK] [--ftz] [--csv FILE] [--wav-dir DIR]\n");
            return 0;
        } else if (a[0] != '-') {
            const int p = rb_preset_find(a);
            if (p < 0 || n_presets == 32) { fprintf(stderr, "rb_bench_render: unknown preset '%s'\n", a); return 2; }
            presets[n_presets++] = p;
        } else {
            fprintf(stderr, "rb_bench_render: unknown option or missing value: %s\n", a);
            return 2;
        }
    }
    if (n_presets == 0)
        for (int p = 0; p < rb_preset_count() && p < 32; p++) presets[n_presets++] = p;
    if (mask && pin_to(mask) != 0) fprintf(stderr, "rb_bench_render: could not set the affinity\n");
    if (ftz && set_ftz() != 0) fprintf(stderr, "rb_bench_render: no flush-to-zero on this CPU\n");

    if (want_figures) {
        printf("preset,quality,kind,rpm,torque_Nm,power_W,ve,map_kPa,rpm_mean,rpm_std\n");
        for (int ip = 0; ip < n_presets; ip++)
            for (int iq = 0; iq < n_qualities; iq++)
                if (figures(presets[ip], qualities[iq], rate) != 0) return 1;
        return 0;
    }
    FILE *csv = csv_path ? fopen(csv_path, "w") : stdout;
    if (!csv) { fprintf(stderr, "rb_bench_render: cannot write %s\n", csv_path); return 1; }
    fprintf(csv, "preset,quality,drive,window_s,cpu_s,x_real,worst_ms,p99_ms,deadline_ms,hash,nonfinite,rpm_end\n");

    const long max_blocks = 20000;
    double *blocks = malloc((size_t)max_blocks * sizeof *blocks), *best_blocks = malloc((size_t)max_blocks * sizeof *blocks);
    if (!blocks || !best_blocks) return 1;
    int status = 0;
    for (int ip = 0; ip < n_presets; ip++)
        for (int iq = 0; iq < n_qualities; iq++)
            for (int id = 0; id < n_drives; id++) {
                const int preset = presets[ip], quality = qualities[iq], drive = drives[id];
                cell_result best = { 0 };
                best.cpu_s = 1e30;
                long n_blocks = 0;
                for (int run = 0; run < runs; run++) {
                    cell_result r;
                    wav_writer *wav = NULL;
                    char path[1024];
                    if (wav_dir && run == 0) {
                        snprintf(path, sizeof path, "%s/%s-%s-%s.wav", wav_dir, rb_preset_key(preset),
                                 QUALITIES[quality], DRIVES[drive]);
                        wav = wav_open(path, (int)rate, 2, WAV_FLOAT32);
                    }
                    if (run_cell(preset, quality, drive, rate, scene, &r, wav, blocks, max_blocks) != 0) {
                        fprintf(stderr, "rb_bench_render: cannot build %s at %s\n", rb_preset_key(preset), QUALITIES[quality]);
                        status = 1;
                        break;
                    }
                    if (wav) wav_close(wav);
                    if (run > 0 && r.hash != best.hash) {
                        fprintf(stderr, "rb_bench_render: %s %s %s: two runs rendered different sound\n",
                                rb_preset_key(preset), QUALITIES[quality], DRIVES[drive]);
                        status = 1;
                    }
                    n_blocks = lround(r.window_s * rate / BLOCK_FRAMES);
                    if (r.cpu_s < best.cpu_s) {
                        best = r;
                        memcpy(best_blocks, blocks, (size_t)n_blocks * sizeof *blocks);
                    }
                }
                if (best.cpu_s > 1e29) continue;
                qsort(best_blocks, (size_t)n_blocks, sizeof *best_blocks, by_value);
                best.worst_ms = best_blocks[n_blocks - 1];
                best.p99_ms = best_blocks[(long)(0.99 * (double)(n_blocks - 1))];
                fprintf(csv, "%s,%s,%s,%.3f,%.4f,%.3f,%.3f,%.3f,%.3f,%016llx,%.0f,%.1f\n", rb_preset_key(preset),
                        QUALITIES[quality], DRIVES[drive], best.window_s, best.cpu_s, best.window_s / best.cpu_s,
                        best.worst_ms, best.p99_ms, best.deadline_ms, (unsigned long long)best.hash,
                        best.nonfinite, best.rpm_end);
                fflush(csv);
                fprintf(stderr, "%-9s %-6s %-5s %5.2fx real time, worst block %.2f ms of %.2f\n", rb_preset_key(preset),
                        QUALITIES[quality], DRIVES[drive], best.window_s / best.cpu_s, best.worst_ms, best.deadline_ms);
            }
    if (csv != stdout) fclose(csv);
    free(blocks);
    free(best_blocks);
    return status;
}
