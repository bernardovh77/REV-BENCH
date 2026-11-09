/*
 * sound.c - rb_cli sound: the engine heard, rendered to a WAV file.
 *
 * Since stage 121 it drives the engine through the public API alone (rb.h),
 * as the desktop app will: it includes no header from core/src (a test checks
 * that). The drives are a driver's script: the key, the pedal, the clutch,
 * the dyno, each set as a parameter between rendered blocks; the core does
 * the rest, the car on the crank and its road load included. Every drive
 * renders bit for bit what the earlier rb_cli did when it set the engine's
 * fields itself (tools/cli_parity.py, docs/stage-121.md).
 */
#include "cli.h"
#include "csv.h"
#include "rb.h"
#include "wav.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Render in blocks of 128 frames, as rb_cli always has. */
#define BLOCK_FRAMES 128

enum { DRIVE_IDLE, DRIVE_SWEEP, DRIVE_HOLD, DRIVE_REV };

/* The listener's places (RB_LISTENER_*), by name. */
static const char *const LISTENERS[RB_LISTENER_COUNT] = { "tailpipe", "behind", "side", "front", "cabin" };

/* The sound's layers (RB_LAYER_*), by name. */
static const char *const LAYERS[3] = { "pipes", "flow", "engine" };

/* The stereo scenes (RB_SCENE_*), by name. */
static const char *const SCENES[RB_SCENE_COUNT] = {
    "asphalt", "grass", "wall", "garage", "dyno", "iso", "passby", "cabin"
};

/* The mufflers (RB_MUFFLER_*), by name, and as the summary says them. */
static const char *const MUFFLERS[3] = { "none", "straight", "chambered" };
static const char *const MUFFLER_SAYS[3] = { "no muffler", "straight-through muffler", "chambered muffler" };

/* RB_LAYER_ bits from a comma-separated list of LAYERS' names; -1 if a name
 * isn't one, or the list is empty. */
static int layers_from(const char *list)
{
    if (!list || !*list) return -1;
    int bits = 0;
    while (*list) {
        const char *end = strchr(list, ',');
        const size_t len = end ? (size_t)(end - list) : strlen(list);
        int found = -1;
        for (int k = 0; k < 3; k++)
            if (strlen(LAYERS[k]) == len && !strncmp(list, LAYERS[k], len)) found = k;
        if (found < 0) return -1;
        bits |= 1 << found;
        list += len + (end != NULL);
    }
    return bits ? bits : -1;
}

#define SOUND_SETTLE   1.5    /* s a held engine runs before the recording starts  */
#define SWEEP_FROM     1000.0 /* rpm                                               */
#define SWEEP_LIMITER  1.0    /* s on the rev limiter before the foot comes off    */
#define SWEEP_LIFT     0.4    /* s coasting in gear before the clutch goes in      */
#define SWEEP_FALL     4.0    /* s at most for the engine to fall back to idle ... */
#define SWEEP_IDLE     1.5    /* ... and this long idling after, to the end        */

/* The neutral revs of --rev, like the recordings of real engines revved
 * standing still (stage 12c): started with the key, then a blip, then floored
 * into the rev limiter, then the foot off and back to idle. */
#define REV_BLIP_AT     3.0   /* s                                               */
#define REV_BLIP        0.25  /* s at REV_BLIP_THROTTLE                          */
#define REV_BLIP_THROTTLE 0.4
#define REV_FLOOR_AT    5.5   /* s: wide open ...                                */
#define REV_FLOOR       1.6   /* ... for this long, into the limiter             */

/* Sets parameter id, which must take the value (the option was checked). */
static void set(rb_sim *sim, int id, double value)
{
    rb_set_param(sim, id, value);
}

static double tlm(const rb_sim *sim, int id)
{
    return rb_get_telemetry(sim, id);
}

/* The options of rb_cli sound. */
typedef struct sound_options {
    const char *out_path, *csv_path, *preset, *snapshots_path;
    double seconds, rpm, throttle, inertia, gain, os, variation, idle_load, seed;
    double overrun_transition, coast_seconds, pull_seconds;
    int seed_given, burn_model, drive, use_float, where, equal, quality, layers, scene;
    int momentum_order, outlet_radiation, muffler, rate_given;
    double rate;
    const char *headers;   /* each header's length, m, comma-separated; NULL: the preset's */
} sound_options;

/* Reads the options; 0, or 2 after an error (said), or 1 for --help. */
static int read_options(int argc, char **argv, sound_options *o)
{
    *o = (sound_options){
        .out_path = "renders/sound.wav", .preset = "i4",
        .seconds = -1.0, .rpm = 3000.0, .throttle = 1.0, .inertia = 3.0,
        .gain = 1.0, .os = 4.0, .variation = 1.0, .overrun_transition = 0.15,
        .coast_seconds = SWEEP_LIFT, .burn_model = RB_BURN_MODEL_CHARGE,
        .drive = DRIVE_IDLE, .where = RB_LISTENER_BEHIND, .quality = -1,
        .layers = RB_LAYER_ALL, .scene = -1, .momentum_order = 2,
        .outlet_radiation = -1, .muffler = -1, .rate = 48000.0,
    };
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "-o") || !strcmp(a, "--out"))
            ok = (o->out_path = cli_take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--csv"))
            ok = (o->csv_path = cli_take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--snapshots"))
            ok = (o->snapshots_path = cli_take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--preset"))
            ok = (o->preset = cli_take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--idle"))
            o->drive = DRIVE_IDLE;
        else if (!strcmp(a, "--sweep"))
            o->drive = DRIVE_SWEEP;
        else if (!strcmp(a, "--hold"))
            o->drive = DRIVE_HOLD;
        else if (!strcmp(a, "--rev"))
            o->drive = DRIVE_REV;
        else if (!strcmp(a, "--rpm"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->rpm);
        else if (!strcmp(a, "--throttle"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->throttle);
        else if (!strcmp(a, "--inertia"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->inertia);
        else if (!strcmp(a, "--coast-seconds"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->coast_seconds)
                 && o->coast_seconds >= 0.0 && o->coast_seconds <= 60.0;
        else if (!strcmp(a, "--pull-seconds"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->pull_seconds)
                 && o->pull_seconds >= 0.0 && o->pull_seconds <= 120.0;
        else if (!strcmp(a, "-s") || !strcmp(a, "--seconds"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->seconds);
        else if (!strcmp(a, "--listener")) {
            const char *v = cli_take_value(argc, argv, &i);
            o->where = -1;
            for (int k = 0; v && k < RB_LISTENER_COUNT; k++)
                if (!strcmp(v, LISTENERS[k])) o->where = k;
            ok = o->where >= 0;
        } else if (!strcmp(a, "--scene")) {
            const char *v = cli_take_value(argc, argv, &i);
            o->scene = -1;
            for (int k = 0; v && k < RB_SCENE_COUNT; k++)
                if (!strcmp(v, SCENES[k])) o->scene = k;
            ok = o->scene >= 0;
        } else if (!strcmp(a, "--layers"))
            ok = (o->layers = layers_from(cli_take_value(argc, argv, &i))) > 0;
        else if (!strcmp(a, "--variation"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->variation);
        else if (!strcmp(a, "--seed")) {
            o->seed_given = 1;
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->seed);
        } else if (!strcmp(a, "--burn-model")) {
            const char *v = cli_take_value(argc, argv, &i);
            ok = v && (!strcmp(v, "charge") || !strcmp(v, "capped"));
            if (ok) o->burn_model = !strcmp(v, "capped") ? RB_BURN_MODEL_CAPPED : RB_BURN_MODEL_CHARGE;
        } else if (!strcmp(a, "--load"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->idle_load);
        else if (!strcmp(a, "--overrun-transition"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->overrun_transition)
                 && o->overrun_transition >= 0.0 && o->overrun_transition <= 2.0;
        else if (!strcmp(a, "--momentum-order")) {
            const char *v = cli_take_value(argc, argv, &i);
            ok = v && (!strcmp(v, "1") || !strcmp(v, "2"));
            if (ok) o->momentum_order = v[0] - '0';
        } else if (!strcmp(a, "--outlet-radiation")) {
            const char *v = cli_take_value(argc, argv, &i);
            ok = v && (!strcmp(v, "unflanged") || !strcmp(v, "constant"));
            if (ok) o->outlet_radiation = !strcmp(v, "unflanged");
        } else if (!strcmp(a, "--muffler")) {
            const char *v = cli_take_value(argc, argv, &i);
            ok = 0;
            for (int k = 0; v && k < 3; k++)
                if (!strcmp(v, MUFFLERS[k])) {
                    o->muffler = k;
                    ok = 1;
                }
        } else if (!strcmp(a, "--equal-headers"))
            o->equal = 1;
        else if (!strcmp(a, "--headers"))
            ok = (o->headers = cli_take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--gain"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->gain);
        else if (!strcmp(a, "--float"))
            o->use_float = 1;
        else if (!strcmp(a, "-r") || !strcmp(a, "--rate"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->rate);
        else if (!strcmp(a, "-x") || !strcmp(a, "--oversample"))
            ok = !cli_to_double(cli_take_value(argc, argv, &i), &o->os);
        else if (!strcmp(a, "--quality"))
            ok = (o->quality = cli_quality_from(cli_take_value(argc, argv, &i))) >= 0;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            cli_usage(stdout);
            return 1;
        } else {
            fprintf(stderr, "rb_cli sound: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli sound: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (o->seconds < 0.0)
        o->seconds = (o->drive == DRIVE_HOLD) ? 5.0 : (o->drive == DRIVE_IDLE) ? 8.0
                   : (o->drive == DRIVE_REV) ? 12.0 : 60.0;
    if (!(o->seconds > 0.0 && o->seconds <= 600.0)) {
        fprintf(stderr, "rb_cli sound: --seconds must be in (0, 600]\n");
        return 2;
    }
    if (!(o->rpm >= 300.0 && o->rpm <= 20000.0) || !(o->throttle >= 0.0 && o->throttle <= 1.0)) {
        fprintf(stderr, "rb_cli sound: --rpm must be in [300, 20000] and --throttle in [0, 1]\n");
        return 2;
    }
    if (!(o->inertia >= 0.01 && o->inertia <= 99.0) || !(o->gain >= 0.0 && o->gain <= 4.0)) {
        fprintf(stderr, "rb_cli sound: --inertia must be in [0.01, 99] kg m^2 and --gain in [0, 4]\n");
        return 2;
    }
    if (!(o->variation >= 0.0 && o->variation <= 4.0)) {
        fprintf(stderr, "rb_cli sound: --variation must be in [0, 4]\n");
        return 2;
    }
    if (o->seed_given && !(o->seed >= 1 && o->seed <= 4294967295.0 && floor(o->seed) == o->seed)) {
        fprintf(stderr, "rb_cli sound: --seed must be an integer in [1, 4294967295]\n");
        return 2;
    }
    if (o->os != floor(o->os) || o->os < 1.0 || o->os > RB_MAX_OVERSAMPLE) {
        fprintf(stderr, "rb_cli sound: --oversample must be an integer in [1, %d]\n", RB_MAX_OVERSAMPLE);
        return 2;
    }
    return 0;
}

/* Builds the engine the options describe in the sim: the preset, then its
 * fields as asked. 0, or 2 after an error (said). */
static int build_engine(rb_sim *sim, const sound_options *o)
{
    const int preset = rb_preset_find(o->preset);
    if (preset < 0) {
        fprintf(stderr, "rb_cli sound: unknown preset '%s' (rb_cli presets lists them)\n", o->preset);
        return 2;
    }
    rb_load_preset(sim, preset);
    int bad = 0;
    if (o->muffler >= 0) bad |= rb_set_engine(sim, RB_ENG_MUFFLER_TYPE, 0, o->muffler) < 0;
    if (o->outlet_radiation >= 0)
        bad |= rb_set_engine(sim, RB_ENG_OUTLET_RADIATION, 0, o->outlet_radiation) < 0;
    const int n = (int)rb_get_engine(sim, RB_ENG_CYLINDERS, 0);
    if (o->headers) {
        const char *v = o->headers;
        int k = 0;
        while (v && *v && k < n) {
            char *end;
            const double length = strtod(v, &end);
            bad |= rb_set_engine(sim, RB_ENG_HEADER_LENGTH, k++, length) < 0;
            v = (*end == ',') ? end + 1 : (*end ? NULL : end);
        }
        if (!(v && !*v && k == n)) {
            fprintf(stderr, "rb_cli sound: missing or bad value for --headers\n");
            return 2;
        }
    }
    if (o->equal) {   /* every header as long as their mean */
        double mean = 0.0;
        for (int i = 0; i < n; i++) mean += rb_get_engine(sim, RB_ENG_HEADER_LENGTH, i) / n;
        for (int i = 0; i < n; i++) bad |= rb_set_engine(sim, RB_ENG_HEADER_LENGTH, i, mean) < 0;
    }
    if (bad) {
        fprintf(stderr, "rb_cli sound: that engine can't be built\n");
        return 2;
    }
    return 0;
}

int cli_sound(int argc, char **argv)
{
    sound_options o;
    const int read = read_options(argc, argv, &o);
    if (read) return read == 1 ? 0 : 2;

    rb_config cfg;
    rb_config_default(&cfg);
    cfg.sample_rate = o.rate;
    cfg.oversample = (int)o.os;
    if (o.quality >= 0 && rb_config_quality(&cfg, o.quality) != RB_OK) {
        fprintf(stderr, "rb_cli sound: no --quality at that --rate\n");
        return 2;
    }
    rb_sim *sim = rb_create(&cfg);
    if (!sim) {
        fprintf(stderr, "rb_cli sound: invalid --rate (must be %g..%g Hz)\n",
                RB_MIN_SAMPLE_RATE, RB_MAX_SAMPLE_RATE);
        return 2;
    }
    if (build_engine(sim, &o)) {
        rb_destroy(sim);
        return 2;
    }
    const int drive = o.drive, scene = o.scene;
    const double rpm = o.rpm, throttle = o.throttle, fs = cfg.sample_rate;
    const double idle_rpm = rb_get_engine(sim, RB_ENG_IDLE_RPM, 0);

    /* the settings: the engine's build-time ones first, then the sound's */
    set(sim, RB_PARAM_BURN_MODEL, o.burn_model);
    if (o.seed_given) set(sim, RB_PARAM_SEED, o.seed);
    set(sim, RB_PARAM_MOMENTUM_ORDER, o.momentum_order);
    set(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    set(sim, RB_PARAM_LISTENER, o.where);
    set(sim, RB_PARAM_LAYERS, o.layers);
    set(sim, RB_PARAM_MASTER_GAIN, o.gain);
    set(sim, RB_PARAM_VARIATION, o.variation);
    set(sim, RB_PARAM_OVERRUN_TRANSITION, o.overrun_transition);
    /* the driver */
    set(sim, RB_PARAM_IGNITION, 1);
    int starter = 0;
    if (drive == DRIVE_HOLD) {
        set(sim, RB_PARAM_HOLD_RPM, rpm);
        set(sim, RB_PARAM_THROTTLE, throttle);
    } else if (drive == DRIVE_SWEEP) {   /* held there to settle, then let go, in gear */
        set(sim, RB_PARAM_HOLD_RPM, SWEEP_FROM);
        set(sim, RB_PARAM_THROTTLE, 1.0);
        set(sim, RB_PARAM_GEAR_INERTIA, o.inertia);
        set(sim, RB_PARAM_CLUTCH, 1);
    } else {   /* --idle and --rev start with the key */
        set(sim, RB_PARAM_HOLD, 0);
        set(sim, RB_PARAM_THROTTLE, 0.0);
        set(sim, RB_PARAM_STARTER, 1);
        starter = 1;
    }

    if (scene >= 0 && rb_set_scene(sim, scene) != RB_OK) {
        fprintf(stderr, "rb_cli sound: stereo scenes require 8..96 kHz output\n");
        rb_destroy(sim);
        return 2;
    }
    const int channels = scene >= 0 ? 2 : 1;
    wav_writer *wav = wav_open(o.out_path, (int)fs, channels, o.use_float ? WAV_FLOAT32 : WAV_PCM16);
    if (!wav) {
        fprintf(stderr, "rb_cli sound: cannot write %s (does the folder exist?)\n", o.out_path);
        rb_destroy(sim);
        return 1;
    }
    /* --snapshots: the engine's snapshots (contracts/snapshot-v2.md), one
     * after another, about 60 a second of the recording */
    FILE *snapshots = NULL;
    static uint64_t snapshot[1024];   /* 8 KiB: far more than 16 cylinders need */
    if (o.snapshots_path && !(snapshots = fopen(o.snapshots_path, "wb"))) {
        fprintf(stderr, "rb_cli sound: cannot write %s\n", o.snapshots_path);
        wav_close(wav);
        rb_destroy(sim);
        return 1;
    }
    const long long snapshot_every = llround(fs / 60.0);
    long long snapshot_next = 0;
    csv_writer *csv = NULL;
    if (o.csv_path) {
        const char *cols[] = { "t_s", "rpm", "throttle", "spark_cut", "fuel_cut", "out_peak", "out_rms",
                               "spl_dB", "crank_inertia_kgm2", "overrun_time_s",
                               "throttle_area_m2", "map_kPa" };
        csv = csv_open(o.csv_path, cols, 12);
        if (!csv) {
            fprintf(stderr, "rb_cli sound: cannot write %s\n", o.csv_path);
            wav_close(wav);
            rb_destroy(sim);
            return 1;
        }
    }

    const int cylinders = (int)rb_get_engine(sim, RB_ENG_CYLINDERS, 0);
    const int muffler = (int)rb_get_engine(sim, RB_ENG_MUFFLER_TYPE, 0);
    const int systems = rb_listener_source_distance(sim, 1) > 0.0 ? 2 : 1;
    printf("sound: %s, %d cylinder%s, %.2f L, %s%s%s; ", rb_preset_name(rb_get_preset(sim)),
           cylinders, cylinders > 1 ? "s" : "", rb_get_engine(sim, RB_ENG_DISPLACEMENT, 0) * 1e3,
           MUFFLER_SAYS[muffler], systems > 1 ? "s" : "", o.equal ? ", equal headers" : "");
    if (scene >= 0)   /* the stereo scene places the listener, not --listener */
        printf("heard in the %s scene (stereo)\n", SCENES[scene]);
    else
        printf("heard from %s: ", LISTENERS[o.where]);
    if (scene >= 0)
        ;
    else if (systems > 1)
        printf("the tailpipes %.2f and %.2f m away", rb_listener_source_distance(sim, 0),
               rb_listener_source_distance(sim, 1));
    else
        printf("the tailpipe %.2f m away", rb_listener_source_distance(sim, 0));
    if (scene < 0)
        printf(", the intake's mouth %.2f m\n", rb_listener_source_distance(sim, 2));
    printf("  physics at %.0f kHz, the pipes at %.0f kHz", 1e-3 / rb_physics_dt(sim), 1e-3 * rb_pipe_rate(sim));
    printf("; burn model %s, variation seed %llu", o.burn_model == RB_BURN_MODEL_CHARGE ? "charge" : "capped",
           (unsigned long long)rb_variation_seed(sim));
    if (o.layers != RB_LAYER_ALL) {
        printf("; only");
        for (int k = 0, first = 1; k < 3; k++)
            if (o.layers & (1 << k)) {
                printf("%s %s", first ? "" : ",", LAYERS[k]);
                first = 0;
            }
    }
    printf("\n");
    float buf[2 * BLOCK_FRAMES];
    if (drive != DRIVE_IDLE) {   /* settle first, held and unrecorded */
        const long blocks = lround(SOUND_SETTLE * fs / BLOCK_FRAMES);
        if (scene >= 0) rb_set_scene_time(sim, -blocks * BLOCK_FRAMES / fs);
        for (long k = 0; k < blocks; k++)
            if (scene >= 0) rb_render_stereo(sim, buf, BLOCK_FRAMES);
            else rb_render_block(sim, buf, BLOCK_FRAMES);
        if (drive == DRIVE_HOLD && tlm(sim, RB_TLM_TORQUE) > 0.0) {
            /* Stage 12c: from here on a dyno holds it, as on stage 9's test
             * bed, its crank free: each firing and each cycle moves it a
             * little, as they do a real engine's. */
            rb_dyno_attach(sim, rpm);
            printf("  on a dyno at %.0f rpm, throttle %.0f %%, after %.1f s to settle\n", rpm,
                   100.0 * throttle, SOUND_SETTLE);
        } else if (drive == DRIVE_HOLD) {
            printf("  held at %.0f rpm, throttle %.0f %% (too little torque for the dyno), after "
                   "%.1f s to settle\n", rpm, 100.0 * throttle, SOUND_SETTLE);
        } else {
            set(sim, RB_PARAM_HOLD, 0);   /* the crank's speed and energy are the held ones: no jump */
        }
    }

    const long long total = llround(o.seconds * fs);
    const double limited0 = tlm(sim, RB_TLM_LIMITED);   /* settling's aren't heard */
    double peak = 0.0, sumsq = 0.0, lo = INFINITY, hi = 0.0;
    double released = -1.0, on_limiter = -1.0, lifted = -1.0, clutch = -1.0, idling = -1.0;
    double fall_from = -1.0, fall_to = -1.0;   /* s: passing 6000 and 2000 rpm, falling */
    const double cuts0 = tlm(sim, RB_TLM_LIMITER_CUTS);
    long long frames = 0;
    int io_error = 0;
    clock_t c0 = clock();
    while (frames < total) {
        const double t = frames / fs;
        /* the driver */
        if ((drive == DRIVE_IDLE || drive == DRIVE_REV) && starter
            && (tlm(sim, RB_TLM_RPM_SMOOTH) > 0.75 * idle_rpm || t > 3.0)) {
            set(sim, RB_PARAM_STARTER, 0);
            starter = 0;
            released = t;
            if (drive == DRIVE_IDLE) set(sim, RB_PARAM_LOAD_TORQUE, o.idle_load);
        }
        if (drive == DRIVE_REV) {
            set(sim, RB_PARAM_THROTTLE, (t >= REV_BLIP_AT && t < REV_BLIP_AT + REV_BLIP) ? REV_BLIP_THROTTLE
                                      : (t >= REV_FLOOR_AT && t < REV_FLOOR_AT + REV_FLOOR) ? 1.0 : 0.0);
            if (on_limiter < 0.0 && tlm(sim, RB_TLM_LIMITER_CUTS) > cuts0) on_limiter = t;
        }
        if (drive == DRIVE_SWEEP) {
            if (on_limiter < 0.0 && tlm(sim, RB_TLM_LIMITER_CUTS) > cuts0) on_limiter = t;
            if (lifted < 0.0 && ((on_limiter >= 0.0 && t >= on_limiter + SWEEP_LIMITER)
                                || (o.pull_seconds > 0.0 && t >= o.pull_seconds))) {
                set(sim, RB_PARAM_THROTTLE, 0.0);
                lifted = t;
            }
            if (clutch < 0.0 && lifted >= 0.0 && t >= lifted + o.coast_seconds) {
                /* The clutch goes in: the car's inertia and the road's load
                 * leave the crank, which keeps its speed. */
                set(sim, RB_PARAM_CLUTCH, 0);
                clutch = t;
            }
            if (clutch >= 0.0 && idling < 0.0
                && (tlm(sim, RB_TLM_RPM_SMOOTH) < idle_rpm + 50.0 || t > clutch + SWEEP_FALL))
                idling = t;
            if (idling >= 0.0 && t >= idling + SWEEP_IDLE) break;
        }
        const double now = tlm(sim, RB_TLM_RPM);
        if (lifted >= 0.0 || (drive == DRIVE_REV && t > REV_FLOOR_AT + REV_FLOOR)) {   /* falling */
            if (fall_from < 0.0 && now < 6000.0) fall_from = t;
            if (fall_to < 0.0 && now < 2000.0) fall_to = t;
        }

        const int n = (int)(total - frames < BLOCK_FRAMES ? total - frames : BLOCK_FRAMES);
        if (scene >= 0) rb_render_stereo(sim, buf, n);
        else rb_render_block(sim, buf, n);
        if (wav_write(wav, buf, (size_t)n) != WAV_OK) io_error = 1;
        for (int i = 0; i < channels * n; i++) {
            if (fabs(buf[i]) > peak) peak = fabs(buf[i]);
            sumsq += (double)buf[i] * buf[i];
        }
        frames += n;
        if (snapshots && frames >= snapshot_next) {
            const int size = rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snapshot, (int)sizeof snapshot);
            if (size <= 0 || fwrite(snapshot, 1, (size_t)size, snapshots) != (size_t)size) io_error = 1;
            snapshot_next += snapshot_every;
        }
        const double after = tlm(sim, RB_TLM_RPM);
        if (after < lo) lo = after;
        if (after > hi) hi = after;
        if (csv) {
            const double rms = tlm(sim, RB_TLM_OUT_RMS);
            const double pa = (o.gain > 0.0) ? rms * RB_OUTPUT_FULL_SCALE_PA / o.gain : 0.0;
            double row[12] = { frames / fs, after, tlm(sim, RB_TLM_THROTTLE), tlm(sim, RB_TLM_SPARK_CUT),
                               tlm(sim, RB_TLM_FUEL_CUT), tlm(sim, RB_TLM_OUT_PEAK), rms,
                               (pa > 0.0) ? 20.0 * log10(pa / 2e-5) : 0.0,
                               tlm(sim, RB_TLM_CRANK_INERTIA), tlm(sim, RB_TLM_OVERRUN_TIME),
                               tlm(sim, RB_TLM_THROTTLE_AREA), tlm(sim, RB_TLM_MAP) * 1e-3 };
            if (csv_row(csv, row) != 0) io_error = 1;
        }
    }
    const double cpu = (double)(clock() - c0) / CLOCKS_PER_SEC;
    const double recorded = frames / fs, rms = frames > 0 ? sqrt(sumsq / (channels * frames)) : 0.0;

    if (drive == DRIVE_IDLE) {
        if (released >= 0.0) printf("  key to START; the starter lets go at %.2f s\n", released);
        printf("  idling at %.0f rpm at the end (%.0f-%.0f rpm over the whole run)\n",
               tlm(sim, RB_TLM_CYCLE_RPM), lo, hi);
    } else if (drive == DRIVE_SWEEP) {
        printf("  wide open from %.0f rpm, %.1f kg m^2 on the crank and the road's load", SWEEP_FROM, o.inertia);
        if (on_limiter >= 0.0) printf(": on the rev limiter after %.1f s", on_limiter);
        else printf(": no rev limiter within %.1f s", recorded);
        if (lifted >= 0.0) printf("; foot off at %.2f s", lifted);
        if (clutch >= 0.0) printf(", clutch in at %.2f s", clutch);
        if (idling >= 0.0) printf(", idle/recovery checkpoint at %.2f s", idling);
        printf("\n");
    } else if (drive == DRIVE_REV) {
        printf("  revved in neutral: a blip at %.1f s, floored at %.1f s", REV_BLIP_AT, REV_FLOOR_AT);
        if (on_limiter >= 0.0) printf(", on the rev limiter at %.2f s", on_limiter);
        printf(", foot off at %.1f s; idling at %.0f rpm at the end\n", REV_FLOOR_AT + REV_FLOOR,
               tlm(sim, RB_TLM_CYCLE_RPM));
    }
    if (drive == DRIVE_SWEEP || drive == DRIVE_REV) {
        printf("  the rev limiter cut %ld sparks", (long)(tlm(sim, RB_TLM_LIMITER_CUTS) - cuts0));
        if (fall_from >= 0.0 && fall_to > fall_from)
            printf("; falling, 6000 to 2000 rpm in %.2f s (%.0f rpm/s)", fall_to - fall_from,
                   4000.0 / (fall_to - fall_from));
        printf("\n");
    }
    const double pa = (o.gain > 0.0) ? rms * RB_OUTPUT_FULL_SCALE_PA / o.gain : 0.0;
    printf("  %.2f s recorded: peak %.3f (%.1f dBFS), rms %.4f: %.1f dB SPL at the listener\n", recorded,
           peak, peak > 0.0 ? 20.0 * log10(peak) : -INFINITY, rms, pa > 0.0 ? 20.0 * log10(pa / 2e-5) : 0.0);
    printf("  soft-limited samples %.0f, non-finite %.0f\n", tlm(sim, RB_TLM_LIMITED) - limited0,
           tlm(sim, RB_TLM_NONFINITE));
    if (cpu > 0.0) printf("  cpu %.1f s (%.2fx real time)\n", cpu, recorded / cpu);

    if (wav_close(wav) != WAV_OK) io_error = 1;
    if (csv && csv_close(csv) != 0) io_error = 1;
    if (snapshots && fclose(snapshots) != 0) io_error = 1;
    rb_destroy(sim);
    printf("wrote %s (%s)%s%s\n", o.out_path, o.use_float ? "32-bit float" : "16-bit PCM, dithered",
           o.csv_path ? " and " : "", o.csv_path ? o.csv_path : "");
    if (io_error) {
        fprintf(stderr, "rb_cli sound: write error\n");
        return 1;
    }
    return 0;
}
