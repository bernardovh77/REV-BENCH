/*
 * rb_cli.c - REV-BENCH command-line harness.
 *
 * Runs the simulation core offline (no real-time deadline) and writes the
 * results to files, so the physics and the sound can be checked and
 * listened to without a browser.
 *
 *   rb_cli info              list parameters and telemetry channels
 *   rb_cli tone [options]    render the stage-1 test tone (a sweep) to WAV
 *   rb_cli blowdown [opts]   stage-3 physics demo: a tank emptying (CSV)
 *   rb_cli motor [opts]      stage-4 physics demo: one cylinder turned by a motor (CSV)
 *   rb_cli breathe [opts]    stage-5 physics demo: the whole engine breathing (CSV)
 *   rb_cli fire [opts]       stage-6 physics demo: the engine running on fuel (CSV)
 *   rb_cli run [opts]        stage-7 demo: the engine started, idled, revved and stopped (CSV)
 *   rb_cli dyno [opts]       stage-9 demo: the engine on a dyno (CSV)
 *   rb_cli pipes [opts]      stage-10 demo: pressure waves in the pipes (CSV)
 *   rb_cli sound [opts]      stages 11-12: the engine's sound, as a listener around it hears it (WAV)
 *
 * The native tools may use the core's internal headers (chamber.h, ...) to
 * exercise pieces of the physics on their own.
 */
#include "chamber.h"
#include "combustion.h"
#include "csv.h"
#include "cylinder.h"
#include "dyno.h"
#include "engine.h"
#include "presets.h"
#include "flow.h"
#include "fuzz.h"
#include "rb.h"
#include "rb_internal.h"   /* rb_cli sound drives the sim's own engine */
#include "wav.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Render in blocks of 128 frames: one Web Audio "render quantum", which
 * is exactly how the browser's AudioWorklet will drive the core. */
#define BLOCK_FRAMES 128

static void usage(FILE *f)
{
    fputs(
        "usage: rb_cli <command> [options]\n"
        "\n"
        "commands:\n"
        "  info                  list parameters, telemetry channels, engine fields and presets\n"
        "  tone [options]        render the stage-1 test tone (a sweep) to a WAV file\n"
        "  blowdown [options]    physics demo: 10 L of air at 5 bar emptying through a 1 cm^2\n"
        "                        hole into the atmosphere; writes p, T and flow as CSV\n"
        "  motor [options]       physics demo: one cylinder with its valves shut, turned at a\n"
        "                        steady speed; writes p, V, T and gamma against crank angle (CSV)\n"
        "  breathe [options]     physics demo: the whole engine breathing, turned by a motor (no\n"
        "                        fuel); volumetric efficiency, plenum pressure, pumping loss\n"
        "  fire [options]        physics demo: the engine running on fuel, held at a set speed by\n"
        "                        a dyno; IMEP, torque, efficiency, peak pressure\n"
        "  run [options]         the engine on its own crank: started with the key, idling,\n"
        "                        revved, into the rev limiter and switched off (CSV)\n"
        "  dyno [options]        the engine on a dyno, crank free: torque and power from a sweep\n"
        "                        or held at each speed (CSV)\n"
        "  presets [KEY] [options]  the engines that come with REV-BENCH, with their layouts\n"
        "                        and firing orders; with a KEY, every field of that engine as\n"
        "                        the API gives it\n"
        "  pipes [options]       physics demo: pressure waves in the pipes. A pulse in a 2 m\n"
        "                        pipe (--pulse), or one cycle of the engine's runner, header,\n"
        "                        exhaust pipe and tailpipe, cell by cell (CSV)\n"
        "  sound [options]       the engine's sound as a listener around the car hears it,\n"
        "                        through the same chain as the browser (WAV)\n"
        "  fuzz [options]        random engines (a preset with random fields), started and\n"
        "                        driven on their own crank: fails if anything stops being a\n"
        "                        number or runs away\n"
        "\n",
        f);
    fputs(
        "tone options:\n"
        "  -o, --out FILE        output WAV                 (default renders/test.wav)\n"
        "      --csv FILE        also write per-block telemetry as CSV\n"
        "  -s, --seconds S       duration in seconds        (default 2)\n"
        "  -r, --rate HZ         audio sample rate          (default 48000)\n"
        "  -x, --oversample N    physics substeps per sample (default 4)\n"
        "      --from HZ         sweep start frequency      (default 110)\n"
        "      --to HZ           sweep end frequency        (default 880)\n"
        "      --level L         tone level 0..1            (default 0.5)\n"
        "      --float           write 32-bit float instead of 16-bit PCM\n"
        "\n"
        "blowdown options:\n"
        "      --csv FILE        output CSV                 (default renders/blowdown.csv)\n"
        "  -s, --seconds S       how long to simulate       (default 1.5)\n"
        "\n"
        "motor options:\n"
        "      --csv FILE        output CSV                 (default renders/motor.csv)\n"
        "      --rpm N           crank speed                (default 3000)\n"
        "      --revs N          revolutions to turn        (default 100)\n"
        "      --bore MM         cylinder bore              (default 86)\n"
        "      --stroke MM       piston stroke              (default 86)\n"
        "      --rod MM          connecting rod length      (default 143)\n"
        "      --cr R            compression ratio          (default 10)\n"
        "\n"
        "breathe options:\n"
        "      --preset KEY      the engine (rb_cli presets lists them) (default i4)\n"
        "      --rpm N           crank speed                (default 3000)\n"
        "      --throttle X      throttle opening, 0..1     (default 1)\n"
        "      --cylinders N     1..16 inline, firing evenly in cylinder order, instead of the\n"
        "                        preset's layout\n"
        "      --plenum L        plenum volume in liters    (default: the preset's)\n"
        "      --intake D/L/C    intake cam: duration (deg), lift (mm), centerline (deg after\n"
        "                        overlap TDC)               (default: the preset's)\n"
        "      --exhaust D/L/C   exhaust cam: same, centerline in deg before overlap TDC\n"
        "                                                   (default: the preset's)\n"
        "      --csv FILE        one cycle of cylinder 1 and the plenum\n"
        "                                                   (default renders/breathe.csv)\n"
        "      --sweep           measure a whole map instead: rpm 800-7000 x 9 throttle\n"
        "                        openings, written to the CSV (default renders/ve.csv)\n"
        "      --walls           heat flow to the cylinder walls (default: adiabatic, as in stage 5)\n"
        "\n",
        f);
    fputs(
        "fire options (also --preset, --rpm, --throttle, --cylinders, --plenum, --cr, --intake,\n"
        "--exhaust):\n"
        "      --spark DEG       a fixed spark advance, degrees before TDC (default: the engine\n"
        "                        computer's, for the best torque)\n"
        "      --afr R           air-fuel ratio by mass     (default 14.6, stoichiometric)\n"
        "      --motored         ignition off, for comparison\n"
        "      --no-walls        no heat flow to the walls\n"
        "      --csv FILE        one cycle of cylinder 1    (default renders/fire.csv)\n"
        "      --spark-sweep     torque and peak pressure for spark 0-50 deg (default renders/spark.csv)\n"
        "      --rpm-sweep       torque and power from 1000 rpm to the rev limit, wide open\n"
        "                        unless --throttle (default renders/power.csv)\n"
        "\n"
        "run options (15 s: start, idle, a blip at 5 s, floored at 8.5 s, off at 13 s):\n"
        "      --csv FILE        every millisecond          (default renders/run.csv)\n"
        "      --load N          torque the crank drives, N m (default 0: in neutral)\n"
        "      --inertia KGM2    crank, flywheel and clutch (default: the preset's)\n"
        "      --idle RPM        idle speed                 (default: the preset's)\n"
        "      --limit RPM       rev limit                  (default: the preset's)\n"
        "      --idle-valve MM2  idle air valve area, fully open (default: the preset's)\n"
        "      --idle-gains P/I  idle valve opening per rpm below idle / learned per rpm per\n"
        "                        second, e.g. 0.0004/0.0006 (default: the preset's)\n"
        "      --preset KEY      the engine                 (default i4)\n"
        "      --cylinders N     1..16 inline, firing evenly, instead of the preset's layout\n"
        "\n"
        "dyno options (a sweep, or --step; wide open unless --throttle):\n"
        "      --preset KEY      the engine                 (default i4)\n"
        "      --from RPM        first speed                (default 1500)\n"
        "      --to RPM          last speed                 (default: 200 below the rev limit)\n"
        "      --rate RPM/S      how fast the sweep climbs  (default 100)\n"
        "      --step RPM        hold each speed in turn, this far apart, instead of sweeping\n"
        "      --throttle X      0..1                       (default 1)\n"
        "      --cr R, --intake D/L/C, --exhaust D/L/C   compression ratio and cams, as for\n"
        "                        breathe                    (default: the preset's)\n"
        "      --csv FILE        a row per cycle (sweep) or per speed (default renders/dyno.csv)\n"
        "\n"
        "presets options:\n"
        "      --csv FILE        also run each one held at 3000 rpm wide open and write one\n"
        "                        cycle of every cylinder's pressure (renders/presets.csv)\n"
        "\n",
        f);
    fputs(
        "pipes options:\n"
        "      --pulse           a 1 kPa pulse in a 2 m pipe of 40 mm, closed at one end and open\n"
        "                        at the other: when it passes, and how it comes back (renders/pulse.csv)\n"
        "      --preset KEY      otherwise, the engine, held and fired (default i4) ...\n"
        "      --rpm N           ... at this speed      (default 3000)\n"
        "      --throttle X      ... and throttle, 0..1 (default 1): cylinder 1's pipes over one\n"
        "                        cycle, every 2 degrees (renders/pipes.csv)\n"
        "      --csv FILE        output CSV\n"
        "\n"
        "sound options (a listener around the car, the anti-alias filter, DC blocker and soft\n"
        "limiter; full scale is 40 Pa, 126 dB SPL):\n"
        "      --preset KEY      the engine                 (default i4)\n"
        "      --idle            started with the key, then left to idle (the default)\n"
        "      --sweep           wide open from 1000 rpm (held there for 1.5 s first) into the\n"
        "                        rev limiter with a car's inertia and the road's load on the\n"
        "                        crank (a pull in second gear), then the foot off, the clutch in,\n"
        "                        and back to idle\n"
        "      --hold            held at --rpm and --throttle (1.5 s to settle), then on a dyno,\n"
        "                        the crank free\n"
        "      --rev             revved in neutral: started with the key, a blip, then floored\n"
        "                        into the rev limiter, then the foot off back to idle\n"
        "      --rpm N           for --hold                 (default 3000)\n"
        "      --throttle X      for --hold, 0..1           (default 1)\n"
        "      --inertia KGM2    for --sweep: crank, flywheel and the car (default 3)\n"
        "      --load N          for --idle: a load on the crank once it runs, N m (default 0)\n"
        "  -s, --seconds S       how long                   (default: 8 idle, 5 hold, 12 rev;\n"
        "                        the sweep ends 1.5 s after it is back at idle)\n"
        "      --muffler TYPE    none (a straight pipe), straight (straight-through, packed) or\n"
        "                        chambered                  (default: the preset's)\n"
        "      --equal-headers   every exhaust header as long as their mean\n"
        "      --headers L,L,...  each cylinder's exhaust header, m (as many as it has cylinders)\n"
        "      --listener WHERE  behind: someone 3 m behind the car (the default); tailpipe: a\n"
        "                        microphone 1 m behind the (right) tailpipe; side: 4 m to its\n"
        "                        side; front: leaning over the bonnet, by the intake's mouth;\n"
        "                        cabin: in the driver's seat\n"
        "      --layers LIST     what of the sound, comma-separated (default all): pipes (the\n"
        "                        pipe ends' flows), flow (the flow's noise), engine (its own,\n"
        "                        mechanical sound)\n"
        "      --variation V     how much no two cycles are alike: 0 every one the same (stage\n"
        "                        12b's sound), 1 a real engine's (the default), up to 4\n"
        "      --gain G          output gain, linear        (default 1)\n"
        "  -o, --out FILE        output WAV                 (default renders/sound.wav)\n"
        "      --float           write 32-bit float instead of 16-bit PCM (dithered)\n"
        "      --csv FILE        a row per 128-frame block: speed, throttle, sound level\n"
        "  -r, --rate HZ, -x, --oversample N    as for tone (default 48000, 4)\n"
        "      --quality Q       low, medium or high: the physics and pipe rates together\n"
        "                        (rb.h: RB_QUALITY_*; high is the default)\n"
        "\n"
        "fuzz options:\n"
        "      --sets N          how many random engines    (default 20)\n"
        "  -s, --seconds S       each one's run             (default 10)\n"
        "      --seed K          the first one's seed; the others follow (default 1)\n"
        "  -x, --oversample N    physics substeps per 48 kHz sample (default 4)\n"
        "      --quality Q       low, medium or high, as for sound\n",
        f);
}

/* Returns the value that follows option argv[*i], or NULL if missing. */
static const char *take_value(int argc, char **argv, int *i)
{
    if (*i + 1 >= argc) return NULL;
    *i += 1;
    return argv[*i];
}

/* The quality levels by name (rb.h: RB_QUALITY_*). */
static const char *const QUALITIES[RB_QUALITY_COUNT] = { "low", "medium", "high" };

/* A quality level's number from its name, or -1. */
static int quality_from(const char *name)
{
    for (int q = 0; name && q < RB_QUALITY_COUNT; q++)
        if (!strcmp(name, QUALITIES[q])) return q;
    return -1;
}

/* Parses a finite number; returns 0 on success, -1 otherwise. */
static int to_double(const char *s, double *out)
{
    char *end;
    double v;

    if (!s) return -1;
    v = strtod(s, &end);
    if (end == s || *end != '\0' || !isfinite(v)) return -1;
    *out = v;
    return 0;
}

static int cmd_info(void)
{
    printf("REV-BENCH core %s\n\nparameters:\n", rb_version());
    printf("  %3s  %-18s %-5s %10s %10s %10s  %s\n",
           "id", "name", "unit", "min", "max", "default", "flags");
    for (int i = 0; i < rb_param_count(); i++) {
        int fl = rb_param_flags(i);
        printf("  %3d  %-18s %-5s %10g %10g %10g  %s%s\n",
               i, rb_param_name(i), rb_param_unit(i),
               rb_param_min(i), rb_param_max(i), rb_param_default(i),
               (fl & RB_PF_LIVE) ? "live" : "",
               (fl & RB_PF_REBUILD) ? "rebuild" : "");
    }
    printf("\ntelemetry:\n");
    for (int i = 0; i < rb_telemetry_count(); i++)
        printf("  %3d  %s\n", i, rb_telemetry_name(i));

    printf("\nengine fields (rb_get_engine, rb_set_engine; values: rb_cli presets KEY):\n");
    printf("  %3s  %-18s %-7s %9s %9s  %-14s %s\n", "id", "name", "unit", "min", "max",
           "one for each", "flags");
    for (int i = 0; i < rb_engine_field_count(); i++) {
        const int fl = rb_engine_field_flags(i);
        char line[200], range[32] = "";
        if (!(fl & RB_EF_READONLY))
            snprintf(range, sizeof range, "%9g %9g", rb_engine_field_min(i), rb_engine_field_max(i));
        int n = snprintf(line, sizeof line, "  %3d  %-18s %-7s %-19s  %-14s %s%s%s", i,
                         rb_engine_field_name(i), rb_engine_field_unit(i), range,
                         (fl & RB_EF_PER_BANK) ? "bank" : (fl & RB_EF_PER_CYLINDER) ? "cylinder"
                         : (fl & RB_EF_IN_ORDER) ? "firing place" : "",
                         (fl & RB_EF_READONLY) ? "read-only " : "",
                         (fl & RB_EF_INTEGER) ? "whole " : "", (fl & RB_EF_LIVE) ? "live" : "");
        while (n > 0 && line[n - 1] == ' ') line[--n] = '\0';   /* no trailing spaces */
        puts(line);
    }
    printf("\ncylinder state (rb_get_cylinder):\n");
    for (int i = 0; i < rb_cylinder_field_count(); i++) {
        const char *unit = rb_cylinder_field_unit(i);
        if (unit[0]) printf("  %3d  %-18s %s\n", i, rb_cylinder_field_name(i), unit);
        else printf("  %3d  %s\n", i, rb_cylinder_field_name(i));
    }
    printf("\npresets (rb_load_preset):\n");
    for (int i = 0; i < rb_preset_count(); i++)
        printf("  %3d  %-18s %s\n", i, rb_preset_key(i), rb_preset_name(i));
    return 0;
}

static int cmd_tone(int argc, char **argv)
{
    const char *out_path = "renders/test.wav";
    const char *csv_path = NULL;
    double seconds = 2.0, f_from = 110.0, f_to = 880.0, level = 0.5, os = 4.0;
    int use_float = 0;
    rb_config cfg;

    rb_config_default(&cfg);

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;

        if (!strcmp(a, "-o") || !strcmp(a, "--out"))
            ok = (out_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "-s") || !strcmp(a, "--seconds"))
            ok = !to_double(take_value(argc, argv, &i), &seconds);
        else if (!strcmp(a, "-r") || !strcmp(a, "--rate"))
            ok = !to_double(take_value(argc, argv, &i), &cfg.sample_rate);
        else if (!strcmp(a, "-x") || !strcmp(a, "--oversample"))
            ok = !to_double(take_value(argc, argv, &i), &os);
        else if (!strcmp(a, "--from"))
            ok = !to_double(take_value(argc, argv, &i), &f_from);
        else if (!strcmp(a, "--to"))
            ok = !to_double(take_value(argc, argv, &i), &f_to);
        else if (!strcmp(a, "--level"))
            ok = !to_double(take_value(argc, argv, &i), &level);
        else if (!strcmp(a, "--float"))
            use_float = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli tone: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli tone: missing or bad value for %s\n", a);
            return 2;
        }
    }

    if (!(seconds > 0.0 && seconds <= 3600.0)) {
        fprintf(stderr, "rb_cli tone: --seconds must be in (0, 3600]\n");
        return 2;
    }
    if (!(f_from > 0.0 && f_to > 0.0)) {
        fprintf(stderr, "rb_cli tone: sweep frequencies must be > 0\n");
        return 2;
    }
    if (os != floor(os) || os < 1.0 || os > RB_MAX_OVERSAMPLE) {
        fprintf(stderr, "rb_cli tone: --oversample must be an integer in [1, %d]\n",
                RB_MAX_OVERSAMPLE);
        return 2;
    }
    cfg.oversample = (int)os;

    rb_sim *sim = rb_create(&cfg);
    if (!sim) {
        fprintf(stderr, "rb_cli tone: invalid --rate (must be %g..%g Hz)\n",
                RB_MIN_SAMPLE_RATE, RB_MAX_SAMPLE_RATE);
        return 2;
    }
    rb_set_param(sim, RB_PARAM_TEST_TONE_LEVEL, level);
    rb_set_param(sim, RB_PARAM_TEST_TONE_HZ, f_from);

    wav_writer *wav = wav_open(out_path, (int)cfg.sample_rate, 1,
                               use_float ? WAV_FLOAT32 : WAV_PCM16);
    if (!wav) {
        fprintf(stderr, "rb_cli tone: cannot write %s (does the folder exist?)\n", out_path);
        rb_destroy(sim);
        return 1;
    }

    csv_writer *csv = NULL;
    if (csv_path) {
        const char *cols[] = { "time_s", "tone_hz", "out_peak", "out_rms" };
        csv = csv_open(csv_path, cols, 4);
        if (!csv) {
            fprintf(stderr, "rb_cli tone: cannot write %s\n", csv_path);
            wav_close(wav);
            rb_destroy(sim);
            return 1;
        }
    }

    const long long total  = llround(seconds * cfg.sample_rate);
    const double    fade_s = seconds > 0.4 ? 0.1 : seconds / 4.0;
    float  buf[BLOCK_FRAMES];
    double peak = 0.0, sumsq = 0.0;
    int    io_error = 0;
    clock_t c0 = clock();

    for (long long done = 0; done < total; ) {
        int    n = (int)(total - done < BLOCK_FRAMES ? total - done : BLOCK_FRAMES);
        double t = (double)done / cfg.sample_rate;

        /* Exponential sweep: the same time is spent on every octave. */
        double f = f_from * pow(f_to / f_from, t / seconds);
        rb_set_param(sim, RB_PARAM_TEST_TONE_HZ, f);
        /* Drop the level near the end; the smoother turns it into a fade. */
        if (t >= seconds - fade_s)
            rb_set_param(sim, RB_PARAM_TEST_TONE_LEVEL, 0.0);

        rb_render_block(sim, buf, n);
        if (wav_write(wav, buf, (size_t)n) != WAV_OK) io_error = 1;

        for (int i = 0; i < n; i++) {
            if (fabs(buf[i]) > peak) peak = fabs(buf[i]);
            sumsq += (double)buf[i] * buf[i];
        }
        if (csv) {
            double row[4] = { t, f,
                              rb_get_telemetry(sim, RB_TLM_OUT_PEAK),
                              rb_get_telemetry(sim, RB_TLM_OUT_RMS) };
            if (csv_row(csv, row) != 0) io_error = 1;
        }
        done += n;
    }

    double cpu = (double)(clock() - c0) / CLOCKS_PER_SEC;
    double nonfinite = rb_get_telemetry(sim, RB_TLM_NONFINITE);

    if (wav_close(wav) != WAV_OK) io_error = 1;
    if (csv && csv_close(csv) != 0) io_error = 1;
    rb_destroy(sim);

    printf("rendered %.3f s: %lld frames at %.0f Hz, physics at %.0f Hz (oversample %d)\n",
           seconds, total, cfg.sample_rate, cfg.sample_rate * cfg.oversample, cfg.oversample);
    printf("sweep %.1f -> %.1f Hz, peak %.3f, rms %.3f, non-finite samples %.0f\n",
           f_from, f_to, peak, total > 0 ? sqrt(sumsq / (double)total) : 0.0, nonfinite);
    if (cpu > 0.0)
        printf("cpu %.3f s (%.0fx faster than real time)\n", cpu, seconds / cpu);
    else
        printf("cpu < 1 ms\n");
    printf("wrote %s (%s)%s%s\n", out_path, use_float ? "32-bit float" : "16-bit PCM, dithered",
           csv_path ? " and " : "", csv_path ? csv_path : "");

    if (io_error) {
        fprintf(stderr, "rb_cli tone: write error\n");
        return 1;
    }
    return 0;
}

/* Stage-3 physics demo: 10 L of air at 5 bar and 300 K empties into the
 * atmosphere through a 1 cm^2 hole. Logs the tank every millisecond. */
static int cmd_blowdown(int argc, char **argv)
{
    static const double air[RB_NSPECIES] = { [RB_AIR] = 1.0 };
    const char *csv_path = "renders/blowdown.csv";
    const double dt = 1.0 / 192000.0;   /* the default physics step */
    double seconds = 1.5;
    double unchoked_at = -1.0, p_unchoked = 0.0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "-s") || !strcmp(a, "--seconds"))
            ok = !to_double(take_value(argc, argv, &i), &seconds);
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli blowdown: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli blowdown: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (!(seconds > 0.0 && seconds <= 60.0)) {
        fprintf(stderr, "rb_cli blowdown: --seconds must be in (0, 60]\n");
        return 2;
    }

    rb_chamber tank, atmosphere;
    rb_orifice hole;
    rb_chamber *all[2] = { &tank, &atmosphere };
    rb_chamber_init(&tank, 10e-3, 5e5, 300.0, air);
    rb_chamber_init_reservoir(&atmosphere, 1e5, 300.0, air);
    rb_orifice_init(&hole, &tank, &atmosphere, 1e-4, 1.0);

    const char *cols[] = { "time_s", "p_bar", "T_K", "mdot_kg_s", "choked" };
    csv_writer *csv = csv_open(csv_path, cols, 5);
    if (!csv) {
        fprintf(stderr, "rb_cli blowdown: cannot write %s (does the folder exist?)\n", csv_path);
        return 1;
    }

    const long steps = lround(seconds / dt);
    for (long n = 0; n < steps; n++) {
        const double t = n * dt, p = tank.p, T = tank.T;
        const int choked = atmosphere.p / p <= rb_nozzle_critical_ratio(rb_chamber_gamma(&tank));
        if (!choked && unchoked_at < 0.0) {
            unchoked_at = t;
            p_unchoked = p;
        }
        rb_network_step(all, 2, &hole, 1, dt);   /* flow from the state at t */
        if (n % 192 == 0) {                       /* every millisecond */
            double row[5] = { t, p / 1e5, T, hole.mdot, choked };
            csv_row(csv, row);
        }
    }
    const int rc = csv_close(csv);

    printf("blowdown: 10 L of air at 5 bar and 300 K, through 1 cm^2 into 1 bar\n");
    if (unchoked_at >= 0.0)
        printf("flow choked until t = %.3f s (tank at %.3f bar)\n", unchoked_at, p_unchoked / 1e5);
    printf("after %.2f s: p = %.4f bar, T = %.1f K\n", seconds, tank.p / 1e5, tank.T);
    printf("wrote %s\n", csv_path);
    return rc ? 1 : 0;
}

/* Stage-4 physics demo: one cylinder with its valves shut, turned at a steady
 * speed by an electric motor ("motoring"). The trapped air is squeezed and
 * released every revolution. Logs the last two revolutions: one 720-degree
 * four-stroke cycle from -360 to 360 degrees, firing TDC in the middle. */
static int cmd_motor(int argc, char **argv)
{
    static const double air[RB_NSPECIES] = { [RB_AIR] = 1.0 };
    const double PI = 3.14159265358979323846;
    const double dt = 1.0 / 192000.0;      /* the default physics step     */
    const double p0 = 1e5, T0 = 300.0;     /* trapped at BDC: 1 bar, 300 K */
    const char *csv_path = "renders/motor.csv";
    double rpm = 3000.0, revs = 100.0, bore = 86.0, stroke = 86.0, rod = 143.0, cr = 10.0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--rpm"))
            ok = !to_double(take_value(argc, argv, &i), &rpm);
        else if (!strcmp(a, "--revs"))
            ok = !to_double(take_value(argc, argv, &i), &revs);
        else if (!strcmp(a, "--bore"))
            ok = !to_double(take_value(argc, argv, &i), &bore);
        else if (!strcmp(a, "--stroke"))
            ok = !to_double(take_value(argc, argv, &i), &stroke);
        else if (!strcmp(a, "--rod"))
            ok = !to_double(take_value(argc, argv, &i), &rod);
        else if (!strcmp(a, "--cr"))
            ok = !to_double(take_value(argc, argv, &i), &cr);
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli motor: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli motor: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (!(rpm >= 100.0 && rpm <= 25000.0)) {
        fprintf(stderr, "rb_cli motor: --rpm must be in [100, 25000]\n");
        return 2;
    }
    if (revs != floor(revs) || revs < 2.0 || revs > 10000.0) {
        fprintf(stderr, "rb_cli motor: --revs must be a whole number in [2, 10000]\n");
        return 2;
    }
    rb_cyl_geom g;
    if (rb_cyl_geom_init(&g, bore * 1e-3, stroke * 1e-3, rod * 1e-3, cr) != 0) {
        fprintf(stderr, "rb_cli motor: impossible engine: the sizes must be positive, the rod "
                        "longer than the stroke, and --cr above 1\n");
        return 2;
    }

    /* A whole, even number of steps per revolution: every revolution ends
     * exactly where it started, and TDC and BDC fall on a step. */
    const long half = lround(30.0 / (rpm * dt));   /* steps per half revolution */
    const long spr = 2 * half;                     /* steps per revolution      */
    const double rpm_used = 60.0 / ((double)spr * dt);
    const long nrev = (long)revs;

    /* Log one row every `stride` steps (about half a degree), with a stride
     * that divides half a revolution so TDC and BDC get a row each. */
    long stride = half / 360 > 1 ? half / 360 : 1;
    while (half % stride) stride--;

    const char *cols[] = { "crank_deg", "piston_mm", "volume_cm3", "p_bar", "T_K", "gamma" };
    csv_writer *csv = csv_open(csv_path, cols, 6);
    if (!csv) {
        fprintf(stderr, "rb_cli motor: cannot write %s (does the folder exist?)\n", csv_path);
        return 1;
    }

    rb_cylinder c;
    rb_cylinder_init(&c, &g, PI, p0, T0, air);   /* BDC */
    const double U0 = c.gas.U;
    double p_tdc = 0.0, T_tdc = 0.0, U_tdc = 0.0, U_end = 0.0, T_end = 0.0;
    double p_first = 0.0, p_last = 0.0;           /* peak pressure, first and last revolution */
    long rows = 0;

    /* Turn `revs` revolutions from BDC, then half a revolution more so the
     * log (the last two revolutions) runs from TDC to TDC. */
    const long total = nrev * spr + half, log_from = total - 2 * spr;
    clock_t c0 = clock();
    for (long n = 0; n <= total; n++) {
        if (n > 0) {
            /* The angle from the step count, wrapped to [-pi, pi): BDC is -pi
             * and TDC exactly 0. Only the angle mod 2 pi matters to the piston. */
            const long k = n % spr;   /* steps into the revolution, from BDC */
            rb_cylinder_move(&c, PI * (double)(k - half) / (double)half);
            if (n <= spr) p_first = fmax(p_first, c.gas.p);
            if (n > (nrev - 1) * spr && n <= nrev * spr) p_last = fmax(p_last, c.gas.p);
        }
        if (n == half) {
            p_tdc = c.gas.p;
            T_tdc = c.gas.T;
            U_tdc = c.gas.U;
        }
        if (n == nrev * spr) {
            U_end = c.gas.U;
            T_end = c.gas.T;
        }
        if (n >= log_from && (n - log_from) % stride == 0) {
            double row[6] = { -360.0 + 180.0 * (double)(n - log_from) / (double)half,
                              rb_cyl_piston_x(&g, c.theta) * 1e3, c.gas.V * 1e6,
                              c.gas.p / 1e5, c.gas.T, rb_chamber_gamma(&c.gas) };
            csv_row(csv, row);
            rows++;
        }
    }
    const double cpu = (double)(clock() - c0) / CLOCKS_PER_SEC;
    const double sim_s = (double)total * dt;
    const int rc = csv_close(csv);

    const double work = U_tdc - U0;   /* done on the gas from BDC to TDC */
    if (fabs(rpm_used - rpm) > 1e-9 * rpm)
        printf("motor: one cylinder with its valves shut, turned at %.1f rpm for %ld revolutions\n"
               "  (%g rpm asked: this is the nearest speed with a whole, even number of steps\n"
               "  per revolution)\n", rpm_used, nrev, rpm);
    else
        printf("motor: one cylinder with its valves shut, turned at %.0f rpm for %ld revolutions\n",
               rpm_used, nrev);
    printf("  bore %.1f mm, stroke %.1f mm, rod %.1f mm, compression ratio %.2f\n",
           g.bore * 1e3, g.stroke * 1e3, g.rod * 1e3, g.cr);
    printf("  swept volume %.1f cm^3, clearance volume %.1f cm^3, mean piston speed %.1f m/s\n",
           g.v_disp * 1e6, g.v_clear * 1e6, 2.0 * g.stroke * rpm_used / 60.0);
    printf("  %ld steps per revolution (%.3f degrees each)\n", spr, 360.0 / (double)spr);
    printf("start, at BDC:  %6.3f bar, %6.1f K (air)\n", p0 / 1e5, T0);
    printf("at TDC:         %6.2f bar, %6.1f K\n", p_tdc / 1e5, T_tdc);
    printf("  textbook p0 CR^1.4 = %.2f bar, T0 CR^0.4 = %.1f K (constant gamma; real air's\n"
           "  gamma falls as it heats up, so the real gas ends up a little lower)\n",
           p0 * pow(g.cr, 1.4) / 1e5, T0 * pow(g.cr, 0.4));
    printf("  the piston puts %.1f J into the gas on the way up; the gas gives it back on the way down\n",
           work);
    printf("after %ld revolutions: %.6f K at BDC, energy drift %.2g J (%.1g of the work per stroke),\n"
           "  peak pressure %.6f bar in the first revolution and %.6f bar in the last\n",
           nrev, T_end, U_end - U0, fabs(U_end - U0) / work, p_first / 1e5, p_last / 1e5);
    if (cpu > 0.0)
        printf("cpu %.3f s for %.2f s of crank time (%.0fx faster than real time)\n",
               cpu, sim_s, sim_s / cpu);
    printf("wrote %s (%ld rows: the last two revolutions, -360 to 360 degrees)\n", csv_path, rows);
    return rc ? 1 : 0;
}

/* ---- breathe: the stage-5 engine, turned by a motor --------------------- */

static rb_engine engine;   /* about 20 KB: not on the stack */

/* Parses a cam as "duration/lift/centerline", e.g. "250/9.5/110": degrees,
 * mm, and degrees from overlap TDC (after it for an intake lobe, before it
 * for an exhaust lobe, the way cam cards give them). */
static int parse_cam(const char *s, rb_valve *v, int intake)
{
    double duration, lift, centerline;
    char extra;
    if (!s || sscanf(s, "%lf/%lf/%lf%c", &duration, &lift, &centerline, &extra) != 3) return -1;
    v->duration = duration;
    v->max_lift = lift * 1e-3;
    v->center = intake ? 360.0 + centerline : 360.0 - centerline;
    return 0;
}

/* A cycle angle the way mechanics say it: "15 BTDC", "55 ABDC", ... */
static const char *dead_centre(double angle, char *buf, size_t size)
{
    const double k = floor(angle / 180.0 + 0.5);   /* nearest dead centre */
    const double rel = angle - 180.0 * k;
    snprintf(buf, size, "%.0f %s%s", fabs(rel), rel < 0.0 ? "B" : "A",
             fmod(k, 2.0) == 0.0 ? "TDC" : "BDC");
    return buf;
}

static void print_cam(const char *name, const rb_valve *v, int intake)
{
    char a[24], b[24];
    printf("  %-8s %3.0f deg, %.1f mm lift, centerline %.0f %s: opens %s, closes %s\n",
           name, v->duration, v->max_lift * 1e3,
           intake ? v->center - 360.0 : 360.0 - v->center, intake ? "ATDC" : "BTDC",
           dead_centre(rb_valve_opens(v), a, sizeof a), dead_centre(rb_valve_closes(v), b, sizeof b));
}

/* The engine a command runs: the preset named by --preset (i4 if none),
 * looked up before the other options so that they can change it. */
static const char *engine_name = "";

static int engine_from_args(int argc, char **argv, const char *cmd, rb_engine_desc *d)
{
    const char *key = "i4";
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--preset")) key = argv[i + 1];
    const int p = rb_preset_find(key);
    if (p < 0) {
        fprintf(stderr, "rb_cli %s: unknown preset '%s' (rb_cli presets lists them)\n", cmd, key);
        return 2;
    }
    rb_preset_desc(p, d);
    engine_name = rb_preset_name(p);
    return 0;
}

/* --cylinders N: an inline engine of N cylinders firing evenly, instead of
 * the preset's layout (0 = keep the preset's). Returns 0, or 2 after an error. */
static int cylinders_option(double n, const char *cmd, rb_engine_desc *d)
{
    if (n == 0.0) return 0;
    if (n != floor(n) || n < 1.0 || n > RB_MAX_CYLINDERS) {
        fprintf(stderr, "rb_cli %s: --cylinders must be a whole number in [1, %d]\n", cmd,
                RB_MAX_CYLINDERS);
        return 2;
    }
    rb_engine_layout_inline(d, (int)n);
    engine_name = "inline engine, evenly firing";
    return 0;
}

/* One operating point: what the dyno and the driver set. The spark is the
 * engine computer's (spark_map) unless the driver fixes it. */
typedef struct op_point {
    double rpm, throttle, spark, afr;
    int    ignition, spark_map;
} op_point;

/* Runs the engine from rest to a steady state at one operating point (to
 * 1e-4: plenty for printing and charts). Returns the cycles it took, or -1. */
static int settle_at(const rb_engine_desc *d, const op_point *op)
{
    rb_engine_init(&engine, d, 1.0 / 192000.0);
    engine.rpm = op->rpm;
    engine.throttle_pos = op->throttle;
    engine.ignition = op->ignition;
    engine.spark = op->spark;
    engine.spark_map = op->spark_map;
    engine.afr = op->afr;
    return rb_engine_settle(&engine, 300, 1e-4);
}

/* Logs one cycle of cylinder 1, from -360 to 360 degrees (overlap TDC to
 * overlap TDC, firing TDC in the middle), about every half degree. */
static int write_cycle(const char *path)
{
    const char *cols[] = { "crank_deg", "piston_mm", "volume_cm3", "p_bar", "T_K", "gamma",
                           "intake_lift_mm", "exhaust_lift_mm", "intake_g_s", "exhaust_g_s",
                           "plenum_kPa", "rpm", "throttle", "burned", "heat_J_deg", "wall_J_deg",
                           "spark_deg", "ignition" };
    csv_writer *csv = csv_open(path, cols, 18);
    if (!csv) return -1;

    const rb_engine_desc *d = &engine.desc;
    const double step_deg = 6.0 * engine.rpm * engine.dt;
    const long stride = lround(0.5 / step_deg) > 1 ? lround(0.5 / step_deg) : 1;
    while (engine.theta < 360.0) rb_engine_step(&engine);   /* to overlap TDC */
    /* Cylinder 1's spark this cycle, in degrees before TDC (fixed at its
     * intake valve closing, before the logged cycle's spark). */
    const double sa = engine.burn[0].spark_angle;
    const double advance = (sa >= 360.0) ? 720.0 - sa : -sa;

    /* The logged angle runs on from about -360 instead of wrapping, so the
     * trace is one continuous line. */
    const double start = rb_engine_cylinder_angle(&engine, 0) - 720.0;
    for (long n = 0; start + n * step_deg <= 360.0; n++) {
        if (n % stride == 0) {
            const double a = rb_engine_cylinder_angle(&engine, 0);
            double m = fmod(a, 360.0);   /* the crank's own angle, -180..180: exact at TDC */
            if (m > 180.0) m -= 360.0;
            const rb_chamber *g = &engine.cyl[0].gas;
            const rb_burn *b = &engine.burn[0];
            double row[18] = { start + n * step_deg,
                               rb_cyl_piston_x(&engine.cyl[0].geom, m * 3.14159265358979323846 / 180.0) * 1e3,
                               g->V * 1e6, g->p / 1e5, g->T, rb_chamber_gamma(g),
                               rb_valve_lift(&d->intake, a) * 1e3, rb_valve_lift(&d->exhaust, a) * 1e3,
                               engine.intake[0].mdot * 1e3, engine.exhaust[0].mdot * 1e3,
                               engine.plenum.p / 1e3, engine.rpm, engine.throttle_pos,
                               /* 0 until this cycle's spark: before it the burn record
                                * still holds the last cycle's 1.0 */
                               start + n * step_deg + advance >= 0.0 ? b->fraction : 0.0,
                               b->heat / step_deg, b->wall_heat / step_deg,
                               advance, engine.ignition };
            csv_row(csv, row);
        }
        rb_engine_step(&engine);
    }
    return csv_close(csv);
}

/* The volumetric efficiency map: rpm x throttle. */
static int sweep(const rb_engine_desc *d, const char *path)
{
    static const double rpms[] = { 800, 1000, 1500, 2000, 2500, 3000, 3500, 4000,
                                   4500, 5000, 5500, 6000, 6500, 7000 };
    static const double throttles[] = { 1.0, 0.5, 0.25, 0.15, 0.1, 0.07, 0.04, 0.02, 0.0 };
    enum { NR = sizeof rpms / sizeof rpms[0], NT = sizeof throttles / sizeof throttles[0] };
    static double ve[NR][NT], map[NR][NT];
    const char *cols[] = { "rpm", "throttle", "ve", "ve_plenum", "map_kPa", "map_min_kPa",
                           "map_max_kPa", "pmep_bar", "air_g_s", "cycles" };
    csv_writer *csv = csv_open(path, cols, 10);
    if (!csv) {
        fprintf(stderr, "rb_cli breathe: cannot write %s (does the folder exist?)\n", path);
        return 1;
    }

    fprintf(stderr, "measuring %d operating points", NR * NT);
    for (int i = 0; i < NR; i++) {
        for (int j = 0; j < NT; j++) {
            const op_point op = { rpms[i], throttles[j], 25.0, RB_AFR_STOICH, 0, 0 };
            const int cycles = settle_at(d, &op);
            const rb_engine_cycle *r = &engine.result;
            double row[10] = { rpms[i], throttles[j], r->ve, r->ve_plenum, r->map / 1e3,
                               r->map_min / 1e3, r->map_max / 1e3, r->pmep / 1e5,
                               r->air_flow * 1e3, cycles };
            csv_row(csv, row);
            ve[i][j] = r->ve;
            map[i][j] = r->map / 1e3;
        }
        fputc('.', stderr);
    }
    fputc('\n', stderr);

    printf("volumetric efficiency (against ambient air), by rpm and throttle opening:\n   rpm");
    for (int j = 0; j < NT; j++) printf(" %5.0f%%", throttles[j] * 100.0);
    for (int i = 0; i < NR; i++) {
        printf("\n%6.0f", rpms[i]);
        for (int j = 0; j < NT; j++) printf("  %5.3f", ve[i][j]);
    }
    printf("\n\nplenum pressure (kPa):\n   rpm");
    for (int j = 0; j < NT; j++) printf(" %5.0f%%", throttles[j] * 100.0);
    for (int i = 0; i < NR; i++) {
        printf("\n%6.0f", rpms[i]);
        for (int j = 0; j < NT; j++) printf("  %5.1f", map[i][j]);
    }
    printf("\n\nwrote %s\n", path);
    return csv_close(csv) ? 1 : 0;
}

static int cmd_breathe(int argc, char **argv)
{
    rb_engine_desc d;
    const char *csv_path = NULL;
    double rpm = 3000.0, throttle = 1.0, cylinders = 0.0, plenum = -1.0;
    int do_sweep = 0;

    if (engine_from_args(argc, argv, "breathe", &d)) return 2;
    d.heat_transfer = 0;   /* stage 5 measured breathing with adiabatic walls */
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--rpm"))
            ok = !to_double(take_value(argc, argv, &i), &rpm);
        else if (!strcmp(a, "--throttle"))
            ok = !to_double(take_value(argc, argv, &i), &throttle);
        else if (!strcmp(a, "--cylinders"))
            ok = !to_double(take_value(argc, argv, &i), &cylinders);
        else if (!strcmp(a, "--preset"))
            ok = take_value(argc, argv, &i) != NULL;   /* already read */
        else if (!strcmp(a, "--plenum"))
            ok = !to_double(take_value(argc, argv, &i), &plenum);
        else if (!strcmp(a, "--intake"))
            ok = !parse_cam(take_value(argc, argv, &i), &d.intake, 1);
        else if (!strcmp(a, "--exhaust"))
            ok = !parse_cam(take_value(argc, argv, &i), &d.exhaust, 0);
        else if (!strcmp(a, "--sweep"))
            do_sweep = 1;
        else if (!strcmp(a, "--walls"))
            d.heat_transfer = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli breathe: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli breathe: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (!(rpm >= 100.0 && rpm <= 20000.0)) {
        fprintf(stderr, "rb_cli breathe: --rpm must be in [100, 20000]\n");
        return 2;
    }
    if (!(throttle >= 0.0 && throttle <= 1.0)) {
        fprintf(stderr, "rb_cli breathe: --throttle must be in [0, 1]\n");
        return 2;
    }
    if (cylinders_option(cylinders, "breathe", &d)) return 2;
    if (plenum >= 0.0) d.plenum_volume = plenum * 1e-3;
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli breathe: impossible engine (check the cams: duration 1..360 deg, "
                        "lift below the valve diameter, centerline within the cycle; plenum > 0)\n");
        return 2;
    }

    if (do_sweep) return sweep(&d, csv_path ? csv_path : "renders/ve.csv");

    rb_engine_init(&engine, &d, 1.0 / 192000.0);
    const double litres = rb_engine_displacement(&engine) * 1e3;
    printf("breathe: %s, %d cylinder%s, %.2f L (%.1f x %.1f mm), turned by a motor at %.0f rpm, "
           "throttle %.0f %%, %s walls\n", engine_name, d.cylinders, d.cylinders > 1 ? "s" : "", litres,
           d.bore * 1e3, d.stroke * 1e3, rpm, throttle * 100.0,
           d.heat_transfer ? "warm" : "adiabatic");
    print_cam("intake", &d.intake, 1);
    print_cam("exhaust", &d.exhaust, 0);
    const double overlap = rb_valve_closes(&d.exhaust) - rb_valve_opens(&d.intake);
    if (overlap > 0.0)
        printf("  overlap  %.0f deg around TDC, with both valves open\n", overlap);
    else
        printf("  no overlap: the exhaust closes %.0f deg before the intake opens\n", -overlap);

    const op_point op = { rpm, throttle, 25.0, RB_AFR_STOICH, 0, 0 };
    const int cycles = settle_at(&d, &op);
    const rb_engine_cycle *r = &engine.result;
    if (cycles < 0)
        printf("did not settle in 200 cycles; the numbers are from the last one\n");
    else
        printf("settled after %d cycles (%.2f s of running)\n", cycles, engine.cycles * 120.0 / rpm);
    printf("  volumetric efficiency  %.3f against ambient air, %.3f against the plenum's\n",
           r->ve, r->ve_plenum);
    printf("  plenum pressure        %.2f kPa (%.2f to %.2f during the cycle)\n",
           r->map / 1e3, r->map_min / 1e3, r->map_max / 1e3);
    printf("  air flow               %.2f g/s\n", r->air_flow * 1e3);
    printf("  pumping loss (PMEP)    %.3f bar; turning the engine takes %.2f kW\n",
           r->pmep / 1e5, -r->net_work * rpm / 120.0 / 1e3);

    if (!csv_path) csv_path = "renders/breathe.csv";
    if (write_cycle(csv_path) != 0) {
        fprintf(stderr, "rb_cli breathe: cannot write %s (does the folder exist?)\n", csv_path);
        return 1;
    }
    printf("wrote %s (cylinder 1 and the plenum over one cycle)\n", csv_path);
    return 0;
}

/* ---- fire: the stage-6 engine, running on fuel ------------------------- */

static void print_point(const rb_engine_cycle *r, double rpm)
{
    const double cycle_s = 120.0 / rpm;
    const double fuel_energy = r->fuel_flow * cycle_s * RB_FUEL_LHV;
    printf("  volumetric efficiency  %.3f        plenum %.1f kPa, exhaust collector %.1f kPa\n",
           r->ve, r->map / 1e3, r->p_exhaust / 1e3);
    printf("  IMEP                   %.2f bar net (%.2f gross, pumping %.2f)\n",
           r->imep_net / 1e5, r->imep_gross / 1e5, r->pmep / 1e5);
    printf("  indicated torque       %.1f N m, power %.1f kW\n", r->torque, r->power / 1e3);
    printf("  friction               %.2f bar (%.1f N m)\n", r->fmep / 1e5, r->torque - r->brake_torque);
    printf("  brake torque           %.1f N m, power %.1f kW (BMEP %.2f bar)\n", r->brake_torque,
           r->brake_power / 1e3, r->bmep / 1e5);
    if (r->fuel_flow > 0.0) {
        printf("  indicated efficiency   %.1f %%       fuel %.2f g/s, %.0f g/kWh, air-fuel %.2f\n",
               100.0 * r->efficiency, r->fuel_flow * 1e3,
               r->power > 0.0 ? r->fuel_flow / r->power * 3.6e9 : 0.0, r->afr);
        printf("  heat to the walls      %.0f %% of the fuel's energy\n",
               100.0 * r->wall_loss / fuel_energy);
    }
    if (r->fuel_flow > 0.0)
        printf("  spark                  %.1f deg before TDC; the burn takes %.1f deg\n", r->spark,
               r->burn_duration);
    printf("  peak pressure          %.1f bar, %.1f deg %s TDC\n", r->p_max / 1e5,
           fabs(r->angle_p_max), r->angle_p_max < 0.0 ? "before" : "after");
    printf("  exhaust gas            %.0f K leaving the valves; the trapped charge is %.1f %% burned gas\n",
           r->t_exhaust, 100.0 * r->residual);
}

/* Torque, peak pressure and where it happens, for one setting swept. */
static int fire_sweep(const rb_engine_desc *d, op_point op, int by_spark, const char *path)
{
    const char *cols[] = { "rpm", "throttle", "spark_deg", "afr", "imep_net_bar", "imep_gross_bar",
                           "pmep_bar", "torque_Nm", "power_kW", "efficiency", "p_max_bar",
                           "angle_p_max", "ve", "t_exhaust_K", "cycles", "fmep_bar", "bmep_bar",
                           "brake_torque_Nm", "brake_power_kW", "burn_deg" };
    csv_writer *csv = csv_open(path, cols, 20);
    if (!csv) {
        fprintf(stderr, "rb_cli fire: cannot write %s (does the folder exist?)\n", path);
        return 1;
    }
    /* rpm: from 1000 to the rev limit, rounded up to the next 500 */
    const double from = by_spark ? 0.0 : 1000.0;
    const double to = by_spark ? 50.0 : 500.0 * ceil(d->rev_limit / 500.0 - 1e-9);
    const double step = by_spark ? 2.5 : 500.0;
    double best = -1e300, best_at = 0.0, brake = -1e300, brake_at = 0.0, power = -1e300, power_at = 0.0;

    if (by_spark) {
        op.spark_map = 0;
        printf("spark sweep at %.0f rpm, torque against spark advance\n", op.rpm);
    } else {
        printf("rpm sweep at %.0f %% throttle, indicated torque and power%s\n",
               op.throttle * 100.0, op.spark_map ? ", spark for the best torque" : "");
    }
    printf("  %8s %9s %9s %9s %10s %9s %10s %7s\n", by_spark ? "spark" : "rpm", "torque", "power",
           "IMEP", "peak p", "at", "brake", "spark");
    for (double x = from; x <= to + 1e-9; x += step) {
        if (by_spark) op.spark = x;
        else op.rpm = x;
        const int cycles = settle_at(d, &op);
        const rb_engine_cycle *r = &engine.result;
        double row[20] = { op.rpm, op.throttle, r->spark, op.afr, r->imep_net / 1e5,
                           r->imep_gross / 1e5, r->pmep / 1e5, r->torque, r->power / 1e3,
                           r->efficiency, r->p_max / 1e5, r->angle_p_max, r->ve, r->t_exhaust,
                           cycles, r->fmep / 1e5, r->bmep / 1e5, r->brake_torque,
                           r->brake_power / 1e3, r->burn_duration };
        csv_row(csv, row);
        printf("  %8.1f %7.1f Nm %6.1f kW %5.2f bar %6.1f bar %5.1f deg %6.1f Nm %5.1f\n", x,
               r->torque, r->power / 1e3, r->imep_net / 1e5, r->p_max / 1e5, r->angle_p_max,
               r->brake_torque, r->spark);
        if (r->torque > best) {
            best = r->torque;
            best_at = x;
        }
        if (r->brake_torque > brake) {
            brake = r->brake_torque;
            brake_at = x;
        }
        if (r->brake_power > power) {
            power = r->brake_power;
            power_at = x;
        }
    }
    if (by_spark)
        printf("best torque (MBT) at %.1f deg before TDC\n", best_at);
    else
        printf("peak torque at %.0f rpm; at the crank (brake), %.0f N m at %.0f rpm and %.0f kW "
               "at %.0f rpm\n", best_at, brake, brake_at, power / 1e3, power_at);
    printf("wrote %s\n", path);
    return csv_close(csv) ? 1 : 0;
}

static int cmd_fire(int argc, char **argv)
{
    rb_engine_desc d;
    const char *csv_path = NULL;
    double cylinders = 0.0, plenum = -1.0;
    op_point op = { 3000.0, 1.0, 25.0, 14.6, 1, 1 };
    int sweep_spark = 0, sweep_rpm = 0;

    if (engine_from_args(argc, argv, "fire", &d)) return 2;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--rpm"))
            ok = !to_double(take_value(argc, argv, &i), &op.rpm);
        else if (!strcmp(a, "--throttle"))
            ok = !to_double(take_value(argc, argv, &i), &op.throttle);
        else if (!strcmp(a, "--spark")) {
            ok = !to_double(take_value(argc, argv, &i), &op.spark);
            op.spark_map = 0;
        } else if (!strcmp(a, "--afr"))
            ok = !to_double(take_value(argc, argv, &i), &op.afr);
        else if (!strcmp(a, "--cylinders"))
            ok = !to_double(take_value(argc, argv, &i), &cylinders);
        else if (!strcmp(a, "--preset"))
            ok = take_value(argc, argv, &i) != NULL;   /* already read */
        else if (!strcmp(a, "--plenum"))
            ok = !to_double(take_value(argc, argv, &i), &plenum);
        else if (!strcmp(a, "--cr"))
            ok = !to_double(take_value(argc, argv, &i), &d.cr);
        else if (!strcmp(a, "--intake"))
            ok = !parse_cam(take_value(argc, argv, &i), &d.intake, 1);
        else if (!strcmp(a, "--exhaust"))
            ok = !parse_cam(take_value(argc, argv, &i), &d.exhaust, 0);
        else if (!strcmp(a, "--motored"))
            op.ignition = 0;
        else if (!strcmp(a, "--no-walls"))
            d.heat_transfer = 0;
        else if (!strcmp(a, "--spark-sweep"))
            sweep_spark = 1;
        else if (!strcmp(a, "--rpm-sweep"))
            sweep_rpm = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli fire: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli fire: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (!(op.rpm >= 300.0 && op.rpm <= 20000.0)) {
        fprintf(stderr, "rb_cli fire: --rpm must be in [300, 20000]\n");
        return 2;
    }
    if (!(op.throttle >= 0.0 && op.throttle <= 1.0)) {
        fprintf(stderr, "rb_cli fire: --throttle must be in [0, 1]\n");
        return 2;
    }
    if (!(op.spark >= -30.0 && op.spark <= 90.0)) {
        fprintf(stderr, "rb_cli fire: --spark must be in [-30, 90] degrees before TDC\n");
        return 2;
    }
    if (!(op.afr >= 6.0 && op.afr <= 30.0)) {
        fprintf(stderr, "rb_cli fire: --afr must be in [6, 30]\n");
        return 2;
    }
    if (cylinders_option(cylinders, "fire", &d)) return 2;
    if (plenum >= 0.0) d.plenum_volume = plenum * 1e-3;
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli fire: impossible engine (check --cr, the cams and the plenum)\n");
        return 2;
    }
    if (sweep_spark) return fire_sweep(&d, op, 1, csv_path ? csv_path : "renders/spark.csv");
    if (sweep_rpm) return fire_sweep(&d, op, 0, csv_path ? csv_path : "renders/power.csv");

    rb_engine_init(&engine, &d, 1.0 / 192000.0);
    printf("fire: %s, %d cylinder%s, %.2f L, held at %.0f rpm by a dyno, throttle %.0f %%\n",
           engine_name, d.cylinders, d.cylinders > 1 ? "s" : "", rb_engine_displacement(&engine) * 1e3,
           op.rpm, op.throttle * 100.0);
    if (op.ignition && op.spark_map)
        printf("  spark for the best torque, air-fuel ratio %.1f (lambda %.2f), walls %s\n",
               op.afr, op.afr / RB_AFR_STOICH, d.heat_transfer ? "at 430 K" : "adiabatic");
    else if (op.ignition)
        printf("  spark %.1f deg before TDC, air-fuel ratio %.1f (lambda %.2f), walls %s\n",
               op.spark, op.afr, op.afr / RB_AFR_STOICH,
               d.heat_transfer ? "at 430 K" : "adiabatic");
    else
        printf("  ignition off: motored, for comparison; walls %s\n",
               d.heat_transfer ? "at 430 K" : "adiabatic");
    const int cycles = settle_at(&d, &op);
    if (cycles < 0)
        printf("did not settle in 300 cycles; the numbers are from the last one\n");
    else
        printf("settled after %d cycles\n", cycles);
    print_point(&engine.result, op.rpm);

    if (!csv_path) csv_path = op.ignition ? "renders/fire.csv" : "renders/fire-motored.csv";
    if (write_cycle(csv_path) != 0) {
        fprintf(stderr, "rb_cli fire: cannot write %s (does the folder exist?)\n", csv_path);
        return 1;
    }
    printf("wrote %s (cylinder 1 over one cycle)\n", csv_path);
    return 0;
}

/* ---- run: the engine on its own crank (stage 7) ---- */

/* The drive: key to START (let go once it runs), idle, a blip of the throttle,
 * floored into the rev limiter, foot off, ignition off. Times in seconds. */
#define RUN_BLIP   5.0
#define RUN_BLIP_S 0.3
#define RUN_FLOOR  8.5
#define RUN_LIFT   10.5
#define RUN_OFF    13.0
#define RUN_END    15.0

static int cmd_run(int argc, char **argv)
{
    rb_engine_desc d;
    const char *csv_path = "renders/run.csv";
    double load = 0.0, cylinders = 0.0, inertia = -1.0, idle = -1.0, limit = -1.0;
    double valve_mm2 = -1.0, gain_p = -1.0, gain_i = -1.0;

    if (engine_from_args(argc, argv, "run", &d)) return 2;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--load"))
            ok = !to_double(take_value(argc, argv, &i), &load);
        else if (!strcmp(a, "--inertia"))
            ok = !to_double(take_value(argc, argv, &i), &inertia);
        else if (!strcmp(a, "--idle"))
            ok = !to_double(take_value(argc, argv, &i), &idle);
        else if (!strcmp(a, "--limit"))
            ok = !to_double(take_value(argc, argv, &i), &limit);
        else if (!strcmp(a, "--idle-valve"))
            ok = !to_double(take_value(argc, argv, &i), &valve_mm2);
        else if (!strcmp(a, "--idle-gains")) {
            const char *v = take_value(argc, argv, &i);
            char tail;
            ok = v && sscanf(v, "%lf/%lf%c", &gain_p, &gain_i, &tail) == 2;
        } else if (!strcmp(a, "--cylinders"))
            ok = !to_double(take_value(argc, argv, &i), &cylinders);
        else if (!strcmp(a, "--preset"))
            ok = take_value(argc, argv, &i) != NULL;   /* already read */
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli run: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli run: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (!(load >= -100.0 && load <= 300.0)) {
        fprintf(stderr, "rb_cli run: --load must be in [-100, 300] N m\n");
        return 2;
    }
    if (cylinders_option(cylinders, "run", &d)) return 2;
    if (inertia >= 0.0) d.inertia = inertia;
    if (idle >= 0.0) d.idle_rpm = idle;
    if (limit >= 0.0) {
        d.rev_limit = limit;
        d.rev_limit_resume = limit - 100.0;
    }
    if (valve_mm2 >= 0.0) d.idle_valve_area = valve_mm2 * 1e-6;
    if (gain_p != -1.0 || gain_i != -1.0) {
        d.idle_gain_p = gain_p;
        d.idle_gain_i = gain_i;
    }
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli run: impossible engine (check --inertia, --idle, --idle-valve, "
                "--idle-gains and --limit)\n");
        return 2;
    }

    const char *cols[] = { "t_s", "rpm", "throttle", "idle_valve", "map_kPa", "starter", "ignition",
                           "spark_cut", "gas_torque_Nm", "friction_torque_Nm", "starter_torque_Nm",
                           "brake_torque_Nm", "imep_bar" };
    csv_writer *csv = csv_open(csv_path, cols, 13);
    if (!csv) {
        fprintf(stderr, "rb_cli run: cannot write %s (does the folder exist?)\n", csv_path);
        return 1;
    }

    rb_engine *e = &engine;
    rb_engine_init(e, &d, 1.0 / 192000.0);
    e->hold_speed = 0;
    e->throttle_pos = 0.0;
    e->ignition = 1;
    e->starter = 1;
    e->load_torque = load;
    printf("run: %s, %d cylinder%s, %.2f L, crank free (%.2f kg m^2), idle %.0f rpm, rev limit %.0f rpm",
           engine_name, d.cylinders, d.cylinders > 1 ? "s" : "", rb_engine_displacement(e) * 1e3, d.inertia,
           d.idle_rpm, d.rev_limit);
    if (load != 0.0) printf(", load %.0f N m", load);
    printf("\n  %5.2f s  ignition on, key to START: the starter cranks it\n", 0.0);

    const long n = lround(RUN_END / e->dt), per_row = 192;   /* a row every millisecond */
    double first_fire = -1.0, released = -1.0, flare = 0.0, stopped = -1.0;
    double idle_lo = INFINITY, idle_hi = 0.0, idle_map = 0.0, idle_valve = 0.0, idle_rpm = 0.0;
    double blip_hi = 0.0, blip_lo = INFINITY, back = -1.0;
    double to_6000 = -1.0, cut_at = -1.0, lim_lo = INFINITY, lim_hi = 0.0;
    double gas = 0.0, fric = 0.0, start = 0.0;
    long idle_n = 0, cuts = 0;
    for (long k = 1; k <= n; k++) {
        const double t = k * e->dt;
        /* the driver */
        e->throttle_pos = (t >= RUN_BLIP && t < RUN_BLIP + RUN_BLIP_S) ? 0.3
                        : (t >= RUN_FLOOR && t < RUN_LIFT) ? 1.0 : 0.0;
        if (t >= RUN_OFF) e->ignition = 0;
        const int cut_before = e->spark_cut;
        rb_engine_step(e);
        if (e->starter && (e->rpm_smooth > 600.0 || t > 3.0)) {   /* it runs (or it won't) */
            e->starter = 0;
            released = t;
            printf("  %5.2f s  starter released at %.0f rpm\n", t, e->rpm);
        }

        /* what happened */
        if (first_fire < 0.0 && e->heat_released > 0.0) {
            first_fire = t;
            printf("  %5.2f s  first firing\n", t);
        }
        if (released > 0.0 && t < released + 1.0 && e->rpm > flare) flare = e->rpm;
        if (t >= RUN_BLIP - 1.0 && t < RUN_BLIP) {
            if (e->rpm < idle_lo) idle_lo = e->rpm;
            if (e->rpm > idle_hi) idle_hi = e->rpm;
            idle_map += e->plenum.p;
            idle_valve += e->idle_valve;
            idle_rpm = e->result.rpm;          /* the last whole cycle's mean */
            idle_n++;
        }
        if (t >= RUN_BLIP && t < RUN_FLOOR) {
            if (e->rpm > blip_hi) blip_hi = e->rpm;
            if (t > RUN_BLIP + RUN_BLIP_S && e->rpm < blip_lo) blip_lo = e->rpm;
            if (back < 0.0 && t > RUN_BLIP + RUN_BLIP_S && e->rpm_smooth < d.idle_rpm + 50.0) back = t;
        }
        if (t >= RUN_FLOOR && t < RUN_LIFT) {
            if (to_6000 < 0.0 && e->rpm >= 6000.0) to_6000 = t;
            if (cut_at < 0.0 && e->spark_cut) cut_at = t;
            if (e->spark_cut && !cut_before) cuts++;
            if (t >= RUN_LIFT - 1.0) {
                if (e->rpm < lim_lo) lim_lo = e->rpm;
                if (e->rpm > lim_hi) lim_hi = e->rpm;
            }
        }
        if (t >= RUN_OFF && stopped < 0.0 && e->omega == 0.0) stopped = t;

        /* the CSV: torques averaged over each millisecond, the rest as they are */
        gas += e->gas_torque;
        fric += e->friction_torque;
        start += e->starter_torque_now;
        if (k % per_row == 0) {
            double row[13] = { t, e->rpm, e->throttle_pos, e->idle_valve, e->plenum.p / 1e3,
                               e->starter, e->ignition, e->spark_cut, gas / per_row,
                               fric / per_row, start / per_row, e->result.brake_torque,
                               e->result.imep_net / 1e5 };
            csv_row(csv, row);
            gas = fric = start = 0.0;
        }
    }

    if (flare > 0.0) printf("           it flares to %.0f rpm and settles\n", flare);
    if (idle_n > 0)
        printf("  %5.2f s  idling at %.0f rpm (%.0f-%.0f over the last second), plenum %.0f kPa, idle valve %.0f %% open\n",
               RUN_BLIP - 1.0, idle_rpm, idle_lo, idle_hi, idle_map / idle_n / 1e3,
               100.0 * idle_valve / idle_n);
    printf("  %5.2f s  a blip, 30 %% throttle for %.1f s: up to %.0f rpm", RUN_BLIP, RUN_BLIP_S, blip_hi);
    if (back > 0.0) printf(", back to idle at %.1f s (lowest %.0f rpm)\n", back, blip_lo);
    else printf(", not back to idle by %.1f s\n", RUN_FLOOR);
    printf("  %5.2f s  floored:", RUN_FLOOR);
    if (to_6000 > 0.0) printf(" 6000 rpm after %.2f s,", to_6000 - RUN_FLOOR);
    if (cut_at > 0.0)
        printf(" on the rev limiter after %.2f s, then %.0f-%.0f rpm (%ld cuts)\n", cut_at - RUN_FLOOR,
               lim_lo, lim_hi, cuts);
    else
        printf(" never reached the rev limiter\n");
    printf("  %5.2f s  foot off\n", RUN_LIFT);
    printf("  %5.2f s  ignition off: ", RUN_OFF);
    if (stopped > 0.0) printf("stopped at %.2f s\n", stopped);
    else printf("still turning at %.0f rpm at %.0f s\n", e->rpm, RUN_END);
    if (csv_close(csv)) {
        fprintf(stderr, "rb_cli run: error writing %s\n", csv_path);
        return 1;
    }
    printf("wrote %s (every millisecond)\n", csv_path);
    return 0;
}

/* ---- presets: the engines that come with REV-BENCH (stage 8) ---- */

/* The firing order, and how far apart the firings are: overall, and on each
 * bank of a V or boxer. */
static void print_firing(const rb_engine_desc *d)
{
    printf("            firing ");
    for (int k = 0; k < d->cylinders; k++)
        printf("%s%d", k ? "-" : "", d->firing_order[k] + 1);
    for (int b = -1; b < (d->banks > 1 ? d->banks : 0); b++) {
        double first = -1.0, prev = -1.0, gaps[RB_MAX_CYLINDERS];
        int n = 0, even = 1;
        for (int k = 0; k < d->cylinders; k++) {
            const int i = d->firing_order[k];
            if (b >= 0 && d->bank[i] != b) continue;
            if (prev < 0.0) first = d->firing[i];
            else gaps[n++] = d->firing[i] - prev;
            prev = d->firing[i];
        }
        gaps[n++] = first + 720.0 - prev;
        for (int k = 1; k < n; k++) even = even && fabs(gaps[k] - gaps[0]) < 1e-6;
        printf(b < 0 ? ", " : "; bank %d ", b + 1);
        if (even) printf("every %.0f deg", gaps[0]);
        else
            for (int k = 0; k < n; k++) printf("%s%.0f", k ? "-" : "", gaps[k]);
    }
    printf("\n");
}

/* Every field of one preset, read through the public API the way the browser
 * reads it. */
static int print_engine(const char *key)
{
    const int p = rb_preset_find(key);
    if (p < 0) {
        fprintf(stderr, "rb_cli presets: unknown preset '%s' (rb_cli presets lists them)\n", key);
        return 2;
    }
    rb_sim *sim = rb_create(NULL);
    if (!sim) {
        fprintf(stderr, "rb_cli presets: out of memory\n");
        return 1;
    }
    rb_load_preset(sim, p);
    printf("%s, %s: every field (rb_get_engine; cylinders and banks count from 0)\n",
           rb_preset_key(p), rb_preset_name(p));
    for (int f = 0; f < rb_engine_field_count(); f++) {
        const char *unit = rb_engine_field_unit(f);
        printf("  %-18s ", rb_engine_field_name(f));
        for (int i = 0; i < rb_engine_field_size(sim, f); i++)
            printf("%s%.6g", i ? ", " : "", rb_get_engine(sim, f, i));
        printf("%s%s\n", unit[0] ? " " : "", unit);
    }
    rb_destroy(sim);
    return 0;
}

static int cmd_presets(int argc, char **argv)
{
    const char *csv_path = NULL, *key = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--csv")) {
            if (!(csv_path = take_value(argc, argv, &i))) {
                fprintf(stderr, "rb_cli presets: missing value for --csv\n");
                return 2;
            }
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(stdout);
            return 0;
        } else if (argv[i][0] != '-' && !key) {
            key = argv[i];
        } else {
            fprintf(stderr, "rb_cli presets: unknown option '%s'\n", argv[i]);
            return 2;
        }
    }
    if (key) {
        if (csv_path) {
            fprintf(stderr, "rb_cli presets: --csv writes every preset, so it takes no KEY\n");
            return 2;
        }
        return print_engine(key);
    }

    csv_writer *csv = NULL;
    if (csv_path) {
        const char *cols[] = { "preset", "cylinder", "bank", "firing_deg", "crank_deg", "p_bar" };
        if (!(csv = csv_open(csv_path, cols, 6))) {
            fprintf(stderr, "rb_cli presets: cannot write %s (does the folder exist?)\n", csv_path);
            return 1;
        }
    }
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        rb_engine_init(&engine, &d, 1.0 / 192000.0);
        printf("  %-9s %-22s %2d cyl %5.1f x %4.1f mm %5.0f cc  CR %4.1f  idle %4.0f, limit %4.0f rpm\n",
               rb_preset_key(p), rb_preset_name(p), d.cylinders, d.bore * 1e3, d.stroke * 1e3,
               rb_engine_displacement(&engine) * 1e6, d.cr, d.idle_rpm, d.rev_limit);
        print_firing(&d);
        if (!csv) continue;

        /* One cycle at 3000 rpm wide open, from cylinder 1's firing TDC. */
        engine.rpm = 3000.0;
        engine.throttle_pos = 1.0;
        engine.ignition = 1;
        rb_engine_settle(&engine, 60, 1e-3);
        while (engine.theta > 1.0) rb_engine_step(&engine);
        double next = 0.0, turned = 0.0;
        while (turned <= 720.0) {
            if (turned >= next) {
                for (int i = 0; i < d.cylinders; i++) {
                    double row[6] = { p, i + 1, d.bank[i] + 1, d.firing[i], turned,
                                      engine.cyl[i].gas.p / 1e5 };
                    csv_row(csv, row);
                }
                next += 2.0;
            }
            const double before = engine.theta;
            rb_engine_step(&engine);
            turned += engine.theta - before + (engine.theta < before ? 720.0 : 0.0);
        }
    }
    if (csv) {
        if (csv_close(csv)) {
            fprintf(stderr, "rb_cli presets: error writing %s\n", csv_path);
            return 1;
        }
        printf("wrote %s (one cycle of every cylinder, held at 3000 rpm wide open)\n", csv_path);
    }
    return 0;
}

/* ---- dyno: the free engine braked by an absorbing dyno (stage 9) ---- */

#define DYNO_COLS 19

/* One row of the dyno CSV: the dyno's reading and the engine's own numbers
 * for the same cycle (or their means over the cycles averaged). */
static void dyno_row(csv_writer *csv, const rb_dyno_reading *r, const rb_engine_cycle *c,
                     double throttle)
{
    double row[DYNO_COLS] = { r->rpm, r->target_rpm, throttle, r->brake_torque,
                              r->brake_power / 1e3, r->absorbed_torque, c->torque,
                              c->imep_net / 1e5, c->fmep / 1e5, 4.0 * 3.14159265358979323846
                              * r->brake_torque / rb_engine_displacement(&engine) / 1e5,
                              c->ve, c->map / 1e3, c->afr, c->p_max / 1e5, c->angle_p_max,
                              c->efficiency, c->pmep / 1e5, c->spark, c->burn_duration };
    csv_row(csv, row);
}

/* Steps the engine on the dyno until it has read `n` more cycles, giving up
 * after `seconds`. Returns the mean of those readings (and of the engine's
 * cycle results, in *mean). */
static rb_dyno_reading dyno_average(rb_dyno *y, int n, double seconds, rb_engine_cycle *mean)
{
    rb_dyno_reading sum = { 0 };
    rb_engine_cycle m = { 0 };
    int got = 0;
    for (long k = 0; k < lround(seconds / engine.dt) && got < n; k++) {
        if (rb_dyno_step(y, &engine)) {
            const rb_dyno_reading *r = &y->reading;
            const rb_engine_cycle *c = &engine.result;
            sum.rpm += r->rpm;
            sum.target_rpm = r->target_rpm;
            sum.brake_torque += r->brake_torque;
            sum.brake_power += r->brake_power;
            sum.absorbed_torque += r->absorbed_torque;
            m.torque += c->torque;
            m.imep_net += c->imep_net;
            m.fmep += c->fmep;
            m.ve += c->ve;
            m.map += c->map;
            m.afr += c->afr;
            m.p_max += c->p_max;
            m.angle_p_max += c->angle_p_max;
            m.efficiency += c->efficiency;
            m.pmep += c->pmep;
            m.spark += c->spark;
            m.burn_duration += c->burn_duration;
            got++;
        }
        rb_engine_step(&engine);
    }
    const double f = got ? 1.0 / got : 0.0;
    sum.rpm *= f; sum.brake_torque *= f; sum.brake_power *= f; sum.absorbed_torque *= f;
    m.torque *= f; m.imep_net *= f; m.fmep *= f; m.ve *= f; m.map *= f; m.afr *= f;
    m.p_max *= f; m.angle_p_max *= f; m.efficiency *= f; m.pmep *= f;
    m.spark *= f; m.burn_duration *= f;
    *mean = m;
    return sum;
}

static int cmd_dyno(int argc, char **argv)
{
    rb_engine_desc d;
    const char *csv_path = "renders/dyno.csv";
    double from = 1500.0, to = -1.0, rate = 100.0, step = 0.0, throttle = 1.0;

    if (engine_from_args(argc, argv, "dyno", &d)) return 2;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--from"))
            ok = !to_double(take_value(argc, argv, &i), &from);
        else if (!strcmp(a, "--to"))
            ok = !to_double(take_value(argc, argv, &i), &to);
        else if (!strcmp(a, "--rate"))
            ok = !to_double(take_value(argc, argv, &i), &rate);
        else if (!strcmp(a, "--step"))
            ok = !to_double(take_value(argc, argv, &i), &step);
        else if (!strcmp(a, "--throttle"))
            ok = !to_double(take_value(argc, argv, &i), &throttle);
        else if (!strcmp(a, "--cr"))
            ok = !to_double(take_value(argc, argv, &i), &d.cr);
        else if (!strcmp(a, "--intake"))
            ok = !parse_cam(take_value(argc, argv, &i), &d.intake, 1);
        else if (!strcmp(a, "--exhaust"))
            ok = !parse_cam(take_value(argc, argv, &i), &d.exhaust, 0);
        else if (!strcmp(a, "--preset"))
            ok = take_value(argc, argv, &i) != NULL;   /* already read */
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli dyno: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli dyno: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli dyno: impossible engine (check --cr, --intake and --exhaust)\n");
        return 2;
    }
    if (!(to > 0.0)) to = d.rev_limit - 200.0;
    if (!(from >= 500.0 && to > from && to < d.rev_limit)) {
        fprintf(stderr, "rb_cli dyno: need 500 <= --from < --to < the rev limit (%.0f rpm)\n",
                d.rev_limit);
        return 2;
    }
    if (!(rate >= 10.0 && rate <= 5000.0) || !(step >= 0.0) || !(throttle >= 0.0 && throttle <= 1.0)) {
        fprintf(stderr, "rb_cli dyno: --rate must be 10-5000 rpm/s, --step >= 0, --throttle 0-1\n");
        return 2;
    }

    const char *cols[DYNO_COLS] = { "rpm", "target_rpm", "throttle", "brake_torque_Nm",
                                    "brake_power_kW", "absorbed_torque_Nm", "torque_Nm",
                                    "imep_bar", "fmep_bar", "bmep_bar", "ve", "map_kPa", "afr",
                                    "p_max_bar", "angle_p_max", "efficiency", "pmep_bar",
                                    "spark_deg", "burn_deg" };
    csv_writer *csv = csv_open(csv_path, cols, DYNO_COLS);
    if (!csv) {
        fprintf(stderr, "rb_cli dyno: cannot write %s (does the folder exist?)\n", csv_path);
        return 1;
    }

    /* Run the engine up held at the first speed, then hand it to the dyno. */
    rb_engine_init(&engine, &d, 1.0 / 192000.0);
    engine.rpm = from;
    engine.throttle_pos = throttle;
    engine.ignition = 1;
    rb_engine_settle(&engine, 100, 1e-3);
    rb_dyno y;
    rb_dyno_init(&y, &engine, from);
    printf("dyno: %s, %d cylinder%s, %.2f L, %.0f %% throttle, %s %.0f to %.0f rpm\n", engine_name,
           d.cylinders, d.cylinders > 1 ? "s" : "", rb_engine_displacement(&engine) * 1e3,
           100.0 * throttle, step > 0.0 ? "held at each speed," : "a sweep,", from, to);
    printf("  %8s %10s %9s %9s %9s %7s\n", "rpm", "brake", "power", "BMEP", "FMEP", "VE");

    double best_t = 0.0, best_t_at = 0.0, best_p = 0.0, best_p_at = 0.0, next_print = from;
    rb_engine_cycle mean;
    if (step > 0.0) {
        /* Each speed in turn: hold it a second, then average 6 cycles. */
        for (double rpm = from; rpm <= to + 1e-9; rpm += step) {
            y.target_rpm = rpm;
            dyno_average(&y, 1000000, 1.0, &mean);
            const rb_dyno_reading r = dyno_average(&y, 6, 5.0, &mean);
            dyno_row(csv, &r, &mean, throttle);
            printf("  %8.0f %7.1f Nm %6.1f kW %5.2f bar %5.2f bar %7.3f\n", r.rpm, r.brake_torque,
                   r.brake_power / 1e3, 4.0 * 3.14159265358979323846 * r.brake_torque
                   / rb_engine_displacement(&engine) / 1e5, mean.fmep / 1e5, mean.ve);
            if (r.brake_torque > best_t) { best_t = r.brake_torque; best_t_at = r.rpm; }
            if (r.brake_power > best_p) { best_p = r.brake_power; best_p_at = r.rpm; }
        }
    } else {
        /* A second at the first speed, then climb, a row every cycle. */
        dyno_average(&y, 1000000, 1.0, &mean);
        rb_dyno_sweep(&y, to, rate);
        const long most = lround(((to - from) / rate + 2.0) / engine.dt);
        for (long k = 0; k < most; k++) {
            if (rb_dyno_step(&y, &engine)) {
                const rb_dyno_reading *r = &y.reading;
                dyno_row(csv, r, &engine.result, throttle);
                if (r->brake_torque > best_t) { best_t = r->brake_torque; best_t_at = r->rpm; }
                if (r->brake_power > best_p) { best_p = r->brake_power; best_p_at = r->rpm; }
                if (r->rpm >= next_print) {
                    printf("  %8.0f %7.1f Nm %6.1f kW %5.2f bar %5.2f bar %7.3f\n", r->rpm,
                           r->brake_torque, r->brake_power / 1e3, 4.0 * 3.14159265358979323846
                           * r->brake_torque / rb_engine_displacement(&engine) / 1e5,
                           engine.result.fmep / 1e5, engine.result.ve);
                    next_print += 500.0;
                }
                if (y.target_rpm >= to && r->rpm >= to - 5.0) break;
            }
            rb_engine_step(&engine);
        }
    }
    printf("peak brake torque %.1f N m at %.0f rpm, peak power %.1f kW at %.0f rpm\n", best_t,
           best_t_at, best_p / 1e3, best_p_at);
    if (csv_close(csv)) {
        fprintf(stderr, "rb_cli dyno: error writing %s\n", csv_path);
        return 1;
    }
    printf("wrote %s\n", csv_path);
    return 0;
}

/* ---- pipes: pressure waves in the 1D pipes (stage 10) ---- */

/* When trace v (n samples, dt apart) peaks (sign +1) or dips (-1) between
 * samples i0 and i1, to a fraction of a sample: a parabola through the three
 * samples around the extreme. */
static double pulse_extreme(const double *v, long i0, long i1, double sign, double dt)
{
    long best = i0 + 1;
    for (long i = i0 + 1; i < i1 - 1; i++)
        if (sign * v[i] > sign * v[best]) best = i;
    const double a = v[best - 1], b = v[best], c = v[best + 1];
    const double den = a - 2.0 * b + c;
    return (best + (den != 0.0 ? 0.5 * (a - c) / den : 0.0)) * dt;
}

/* A 2 m pipe of still air, 40 mm across, 64 cells, closed at x = 0 and open
 * to the air at x = 2 m. A pulse of 1 kPa starts against the closed end (half
 * a bump 20 cm wide: its mirror image in the wall makes it whole), runs along
 * the pipe and comes back from the open end inverted. Stepped at the engine's
 * pipe rate, 48 kHz; every cell every step for 20 ms. */
static int pipes_pulse(const char *path)
{
    static const double air[RB_NSPECIES] = { [RB_AIR] = 1.0 };
    static rb_chamber cell[64], outside;
    static rb_face face[63];
    static rb_pipe pipe;
    static double at_half[960], at_one_half[960];   /* the pressure 0.5 m and 1.5 m along */
    const double dt = 1.0 / 48000.0, p0 = 101325.0, T0 = 300.0;
    const long steps = 960;                          /* 20 ms */

    rb_chamber_init_reservoir(&outside, p0, T0, air);
    rb_pipe_init(&pipe, cell, face, 64, 2.0, 0.04, p0, T0, air, T0, 0.0);
    rb_pipe_join(&pipe, 1, &outside, 0.0, 0);
    for (int k = 0; k < 64; k++) {   /* a sound wave: squeezed isentropically */
        const double x = (k + 0.5) * pipe.dx, p = p0 + 1000.0 * exp(-(x / 0.2) * (x / 0.2));
        const double g = rb_chamber_gamma(&cell[k]);
        rb_chamber_init(&cell[k], cell[k].V, p, T0 * pow(p / p0, (g - 1.0) / g), air);
    }
    const double c = rb_chamber_sound_speed(&cell[40]);

    const char *cols[] = { "time_ms", "x_m", "p_Pa", "u_m_s" };
    csv_writer *csv = csv_open(path, cols, 4);
    if (!csv) {
        fprintf(stderr, "rb_cli pipes: cannot write %s (does the folder exist?)\n", path);
        return 1;
    }
    for (long n = 0; n < steps; n++) {
        rb_pipe_clear(&pipe, 0);
        rb_pipe_flows(&pipe, 0, dt);
        rb_pipe_integrate(&pipe, 0, dt);
        rb_pipe_accelerate(&pipe, 0, dt);
        for (int k = 0; k < 64; k++) {
            double row[4] = { (n + 1) * dt * 1e3, (k + 0.5) * pipe.dx, cell[k].p - p0,
                              rb_pipe_cell_velocity(&pipe, k) };
            csv_row(csv, row);
        }
        at_half[n] = cell[15].p - p0;        /* (15 + 0.5) / 64 * 2 m = 0.484 m */
        at_one_half[n] = cell[47].p - p0;    /* 1.484 m: one metre on           */
    }
    const int rc = csv_close(csv);

    /* The pulse passes 0.484 m, then 1.484 m; it comes back from the open
     * end as a dip and passes 1.484 m again. */
    const double t1 = pulse_extreme(at_half, 1, 192, 1.0, dt) + dt;
    const double t2 = pulse_extreme(at_one_half, 96, 288, 1.0, dt) + dt;
    const double t3 = pulse_extreme(at_one_half, 288, 480, -1.0, dt) + dt;
    const double end = 1.484 + 0.5 * c * (t3 - t2);   /* where it turned back */
    printf("pipes --pulse: 1 kPa in a 2 m pipe of still air at %.0f K, closed at 0 and open at 2 m\n", T0);
    printf("  speed of sound c        %.1f m/s\n", c);
    printf("  passes 0.484 m at %.3f ms and 1.484 m at %.3f ms: a metre in %.4f ms\n",
           t1 * 1e3, t2 * 1e3, (t2 - t1) * 1e3);
    printf("  L / c = %.4f ms: the pulse runs at %.1f m/s, %+.2f %% of c\n", 1e3 / c, 1.0 / (t2 - t1),
           100.0 * (1.0 / (t2 - t1) / c - 1.0));
    printf("  comes back inverted, at %.3f ms: it turned %.1f mm beyond the pipe's end\n",
           t3 * 1e3, (end - 2.0) * 1e3);
    printf("    (the end correction, 0.3 diameters: %.0f mm)\n", 0.3 * 0.04 * 1e3);
    printf("wrote %s\n", path);
    return rc ? 1 : 0;
}

/* The engine held and fired: one cycle of cylinder 1's pipes, cell by cell,
 * every 2 degrees, from -360 to 360 (overlap TDC to overlap TDC, firing TDC
 * in the middle). Pipes: 0 its runner (x from the plenum), 1 its header
 * (from the valve), 2 its exhaust pipe (from the collector), 3 its tailpipe
 * (from the muffler), 4 its muffler (from its inlet), 5 the snorkel (from its
 * mouth); pipe -1 holds the volumes, cell 0 the cylinder, 1 the plenum, 2 the
 * collector, 3 the air box. */
static int pipes_engine(const rb_engine_desc *d, const op_point *op, const char *path)
{
    printf("pipes: %s held at %.0f rpm by a dyno, throttle %.0f %%\n", engine_name, op->rpm,
           op->throttle * 100.0);
    const int cycles = settle_at(d, op);
    if (cycles < 0) printf("did not settle in 300 cycles; the cycle is the next one\n");
    else printf("settled after %d cycles\n", cycles);
    const rb_engine_cycle *r = &engine.result;
    printf("  volumetric efficiency %.3f, brake torque %.1f N m; exhaust collector %.1f kPa, "
           "plenum %.1f kPa\n", r->ve, r->brake_torque, r->p_exhaust / 1e3, r->map / 1e3);

    const int c = d->collector[0], s = (engine.systems == 2) ? c : 0;
    const rb_pipe *pipes[6] = { d->runner_length > 0.0 ? &engine.runner[0] : NULL, &engine.header[0],
                                &engine.exhaust_pipe[c], &engine.tailpipe[s], &engine.muffler[s],
                                &engine.snorkel };
    const rb_chamber *volumes[4] = { &engine.cyl[0].gas, &engine.plenum, &engine.collector[c],
                                     &engine.airbox };
    const char *cols[] = { "crank_deg", "pipe", "cell", "x_m", "p_kPa", "T_K", "u_m_s",
                           "intake_lift_mm", "exhaust_lift_mm" };
    csv_writer *csv = csv_open(path, cols, 9);
    if (!csv) {
        fprintf(stderr, "rb_cli pipes: cannot write %s (does the folder exist?)\n", path);
        return 1;
    }
    const double step_deg = 6.0 * engine.rpm * engine.dt;
    while (engine.theta < 360.0) rb_engine_step(&engine);   /* to overlap TDC */
    const double start = rb_engine_cylinder_angle(&engine, 0) - 720.0;
    double next = -360.0;
    for (long n = 0; start + n * step_deg <= 360.0; n++) {
        const double angle = start + n * step_deg;
        if (angle >= next) {
            next += 2.0;
            const double a = rb_engine_cylinder_angle(&engine, 0);
            const double in = rb_valve_lift(&d->intake, a) * 1e3, ex = rb_valve_lift(&d->exhaust, a) * 1e3;
            for (int j = 0; j < 4; j++) {
                double row[9] = { angle, -1, j, 0.0, volumes[j]->p / 1e3, volumes[j]->T, 0.0, in, ex };
                csv_row(csv, row);
            }
            for (int j = 0; j < 6; j++) {
                const rb_pipe *p = pipes[j];
                for (int k = 0; p && k < p->cells; k++) {
                    double row[9] = { angle, j, k, (k + 0.5) * p->dx, p->cell[k].p / 1e3,
                                      p->cell[k].T, rb_pipe_cell_velocity(p, k), in, ex };
                    csv_row(csv, row);
                }
            }
        }
        rb_engine_step(&engine);
    }
    printf("  cylinder 1's pipes: runner %d cells, header %d, exhaust pipe %d, muffler %d, "
           "tailpipe %d; the snorkel %d\n", pipes[0] ? pipes[0]->cells : 0, pipes[1]->cells,
           pipes[2]->cells, pipes[4]->cells, pipes[3]->cells, pipes[5]->cells);
    printf("wrote %s\n", path);
    return csv_close(csv) ? 1 : 0;
}

static int cmd_pipes(int argc, char **argv)
{
    rb_engine_desc d;
    const char *csv_path = NULL;
    op_point op = { 3000.0, 1.0, 25.0, 14.6, 1, 1 };
    int pulse = 0;

    if (engine_from_args(argc, argv, "pipes", &d)) return 2;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--pulse"))
            pulse = 1;
        else if (!strcmp(a, "--preset"))
            ok = take_value(argc, argv, &i) != NULL;   /* already read */
        else if (!strcmp(a, "--rpm"))
            ok = !to_double(take_value(argc, argv, &i), &op.rpm);
        else if (!strcmp(a, "--throttle"))
            ok = !to_double(take_value(argc, argv, &i), &op.throttle);
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli pipes: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli pipes: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (pulse) return pipes_pulse(csv_path ? csv_path : "renders/pulse.csv");
    if (!(op.rpm >= 300.0 && op.rpm <= 20000.0)) {
        fprintf(stderr, "rb_cli pipes: --rpm must be in [300, 20000]\n");
        return 2;
    }
    if (!(op.throttle >= 0.0 && op.throttle <= 1.0)) {
        fprintf(stderr, "rb_cli pipes: --throttle must be in [0, 1]\n");
        return 2;
    }
    return pipes_engine(&d, &op, csv_path ? csv_path : "renders/pipes.csv");
}

/* ---- sound: what a listener around the car hears (stages 11 and 12) ---- */

enum { DRIVE_IDLE, DRIVE_SWEEP, DRIVE_HOLD, DRIVE_REV };

/* The listener's places (RB_HEAR_*, audio.h), by name. */
static const char *const LISTENERS[RB_HEAR_COUNT] = { "tailpipe", "behind", "side", "front", "cabin" };

/* The sound's layers (RB_LAYER_*, rb_internal.h), by name. */
static const char *const LAYERS[3] = { "pipes", "flow", "engine" };

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

/* The mufflers (RB_MUFFLER_*, rb.h), by name, and as the summary says them. */
static const char *const MUFFLERS[3] = { "none", "straight", "chambered" };
static const char *const MUFFLER_SAYS[3] = { "no muffler", "straight-through muffler", "chambered muffler" };

#define SOUND_SETTLE   1.5    /* s a held engine runs before the recording starts  */
#define SWEEP_FROM     1000.0 /* rpm                                               */
#define SWEEP_LIMITER  1.0    /* s on the rev limiter before the foot comes off    */
#define SWEEP_LIFT     0.4    /* s coasting in gear before the clutch goes in      */
#define SWEEP_FALL     4.0    /* s at most for the engine to fall back to idle ... */
#define SWEEP_IDLE     1.5    /* ... and this long idling after, to the end        */

/* The car on --sweep (stage 12c): its weight in the gear the pull is in is
 * the crank's inertia less the engine's own; with the wheels' radius that
 * gives the gearing, and so the road speed and the road's load on the crank:
 * rolling resistance and the air's drag. */
#define CAR_MASS     1300.0   /* kg                                              */
#define CAR_WHEEL    0.31     /* m: the driven wheels' rolling radius            */
#define CAR_ROLLING  0.012    /* rolling resistance coefficient                  */
#define CAR_CDA      0.70     /* m^2: drag coefficient times frontal area        */
#define CAR_AIR      1.2      /* kg/m^3                                          */

/* The road's load on the crank (N m) at crank speed w (rad/s), the car's
 * share of the crank's inertia `j_car` (kg m^2): none without a car. */
static double road_load(double w, double j_car)
{
    if (!(j_car > 0.0)) return 0.0;
    const double ratio = CAR_WHEEL * sqrt(CAR_MASS / j_car);   /* crank turns per wheel turn */
    const double v = w * CAR_WHEEL / ratio;
    return (CAR_ROLLING * CAR_MASS * 9.81 + 0.5 * CAR_AIR * CAR_CDA * v * v) * CAR_WHEEL / ratio;
}

/* The neutral revs of --rev, like the recordings of real engines revved
 * standing still (stage 12c): started with the key, then a blip, then floored
 * into the rev limiter, then the foot off and back to idle. */
#define REV_BLIP_AT     3.0   /* s                                               */
#define REV_BLIP        0.25  /* s at REV_BLIP_THROTTLE                          */
#define REV_BLIP_THROTTLE 0.4
#define REV_FLOOR_AT    5.5   /* s: wide open ...                                */
#define REV_FLOOR       1.6   /* ... for this long, into the limiter             */

static int cmd_sound(int argc, char **argv)
{
    rb_engine_desc d;
    const char *out_path = "renders/sound.wav", *csv_path = NULL;
    double seconds = -1.0, rpm = 3000.0, throttle = 1.0, inertia = 3.0;
    double gain = 1.0, os = 4.0, variation = 1.0, idle_load = 0.0;
    rb_dyno dyno;
    int drive = DRIVE_IDLE, use_float = 0, where = RB_HEAR_BEHIND, equal = 0, quality = -1;
    int layers = RB_LAYER_ALL;
    rb_config cfg;

    rb_config_default(&cfg);
    if (engine_from_args(argc, argv, "sound", &d)) return 2;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "-o") || !strcmp(a, "--out"))
            ok = (out_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--preset"))
            ok = take_value(argc, argv, &i) != NULL;   /* already read */
        else if (!strcmp(a, "--idle"))
            drive = DRIVE_IDLE;
        else if (!strcmp(a, "--sweep"))
            drive = DRIVE_SWEEP;
        else if (!strcmp(a, "--hold"))
            drive = DRIVE_HOLD;
        else if (!strcmp(a, "--rev"))
            drive = DRIVE_REV;
        else if (!strcmp(a, "--rpm"))
            ok = !to_double(take_value(argc, argv, &i), &rpm);
        else if (!strcmp(a, "--throttle"))
            ok = !to_double(take_value(argc, argv, &i), &throttle);
        else if (!strcmp(a, "--inertia"))
            ok = !to_double(take_value(argc, argv, &i), &inertia);
        else if (!strcmp(a, "-s") || !strcmp(a, "--seconds"))
            ok = !to_double(take_value(argc, argv, &i), &seconds);
        else if (!strcmp(a, "--listener")) {
            const char *v = take_value(argc, argv, &i);
            where = -1;
            for (int k = 0; v && k < RB_HEAR_COUNT; k++)
                if (!strcmp(v, LISTENERS[k])) where = k;
            ok = where >= 0;
        }
        else if (!strcmp(a, "--layers"))
            ok = (layers = layers_from(take_value(argc, argv, &i))) > 0;
        else if (!strcmp(a, "--variation"))
            ok = !to_double(take_value(argc, argv, &i), &variation);
        else if (!strcmp(a, "--load"))
            ok = !to_double(take_value(argc, argv, &i), &idle_load);
        else if (!strcmp(a, "--muffler")) {
            const char *v = take_value(argc, argv, &i);
            ok = 0;
            for (int k = 0; v && k < 3; k++)
                if (!strcmp(v, MUFFLERS[k])) {
                    d.muffler_type = k;
                    ok = 1;
                }
        }
        else if (!strcmp(a, "--equal-headers"))
            equal = 1;
        else if (!strcmp(a, "--headers")) {   /* each header's length, m, comma-separated */
            const char *v = take_value(argc, argv, &i);
            int k = 0;
            while (v && *v && k < d.cylinders) {
                char *end;
                d.header_length[k++] = strtod(v, &end);
                v = (*end == ',') ? end + 1 : (*end ? NULL : end);
            }
            ok = v && !*v && k == d.cylinders;
        }
        else if (!strcmp(a, "--gain"))
            ok = !to_double(take_value(argc, argv, &i), &gain);
        else if (!strcmp(a, "--float"))
            use_float = 1;
        else if (!strcmp(a, "-r") || !strcmp(a, "--rate"))
            ok = !to_double(take_value(argc, argv, &i), &cfg.sample_rate);
        else if (!strcmp(a, "-x") || !strcmp(a, "--oversample"))
            ok = !to_double(take_value(argc, argv, &i), &os);
        else if (!strcmp(a, "--quality"))
            ok = (quality = quality_from(take_value(argc, argv, &i))) >= 0;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "rb_cli sound: unknown option '%s'\n", a);
            return 2;
        }
        if (!ok) {
            fprintf(stderr, "rb_cli sound: missing or bad value for %s\n", a);
            return 2;
        }
    }
    if (seconds < 0.0)
        seconds = (drive == DRIVE_HOLD) ? 5.0 : (drive == DRIVE_IDLE) ? 8.0 : (drive == DRIVE_REV) ? 12.0 : 60.0;
    if (!(seconds > 0.0 && seconds <= 600.0)) {
        fprintf(stderr, "rb_cli sound: --seconds must be in (0, 600]\n");
        return 2;
    }
    if (!(rpm >= 300.0 && rpm <= 20000.0) || !(throttle >= 0.0 && throttle <= 1.0)) {
        fprintf(stderr, "rb_cli sound: --rpm must be in [300, 20000] and --throttle in [0, 1]\n");
        return 2;
    }
    if (!(inertia >= 0.01 && inertia <= 100.0) || !(gain >= 0.0 && gain <= 4.0)) {
        fprintf(stderr, "rb_cli sound: --inertia must be in [0.01, 100] kg m^2 and --gain in [0, 4]\n");
        return 2;
    }
    if (!(variation >= 0.0 && variation <= 4.0)) {
        fprintf(stderr, "rb_cli sound: --variation must be in [0, 4]\n");
        return 2;
    }
    if (os != floor(os) || os < 1.0 || os > RB_MAX_OVERSAMPLE) {
        fprintf(stderr, "rb_cli sound: --oversample must be an integer in [1, %d]\n", RB_MAX_OVERSAMPLE);
        return 2;
    }
    cfg.oversample = (int)os;
    if (quality >= 0 && rb_config_quality(&cfg, quality) != RB_OK) {
        fprintf(stderr, "rb_cli sound: no --quality at that --rate\n");
        return 2;
    }
    const double own_inertia = d.inertia;   /* the engine's own: crank, flywheel, clutch */
    if (drive == DRIVE_SWEEP) d.inertia = inertia;
    const double car_inertia = d.inertia - own_inertia;
    if (equal) {   /* every header as long as their mean */
        double mean = 0.0;
        for (int i = 0; i < d.cylinders; i++) mean += d.header_length[i] / d.cylinders;
        for (int i = 0; i < d.cylinders; i++) d.header_length[i] = mean;
    }
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli sound: that engine can't be built\n");
        return 2;
    }

    rb_sim *sim = rb_create(&cfg);
    if (!sim) {
        fprintf(stderr, "rb_cli sound: invalid --rate (must be %g..%g Hz)\n",
                RB_MIN_SAMPLE_RATE, RB_MAX_SAMPLE_RATE);
        return 2;
    }
    rb_engine *e = &sim->engine;
    rb_engine_init_at(e, &d, sim->dt, sim->cfg.pipe_rate);   /* at the sim's quality */
    sim->source = RB_SOURCE_ENGINE;
    sim->hear = where;
    sim->layers = layers;
    rb_sim_listen(sim);
    rb_set_param(sim, RB_PARAM_MASTER_GAIN, gain);
    e->ignition = 1;
    e->variation = variation;
    if (drive == DRIVE_HOLD) {
        e->rpm = rpm;
        e->throttle_pos = throttle;
    } else if (drive == DRIVE_SWEEP) {   /* held there to settle, then let go */
        e->rpm = SWEEP_FROM;
        e->throttle_pos = 1.0;
    } else {   /* --idle and --rev start with the key */
        e->hold_speed = 0;
        e->throttle_pos = 0.0;
        e->starter = 1;
    }

    wav_writer *wav = wav_open(out_path, (int)cfg.sample_rate, 1, use_float ? WAV_FLOAT32 : WAV_PCM16);
    if (!wav) {
        fprintf(stderr, "rb_cli sound: cannot write %s (does the folder exist?)\n", out_path);
        rb_destroy(sim);
        return 1;
    }
    csv_writer *csv = NULL;
    if (csv_path) {
        const char *cols[] = { "t_s", "rpm", "throttle", "spark_cut", "fuel_cut", "out_peak", "out_rms",
                               "spl_dB" };
        csv = csv_open(csv_path, cols, 8);
        if (!csv) {
            fprintf(stderr, "rb_cli sound: cannot write %s\n", csv_path);
            wav_close(wav);
            rb_destroy(sim);
            return 1;
        }
    }

    printf("sound: %s, %d cylinder%s, %.2f L, %s%s%s; heard from %s: ", engine_name,
           d.cylinders, d.cylinders > 1 ? "s" : "", rb_engine_displacement(e) * 1e3,
           MUFFLER_SAYS[d.muffler_type], e->systems > 1 ? "s" : "", equal ? ", equal headers" : "",
           LISTENERS[where]);
    if (e->systems > 1)
        printf("the tailpipes %.2f and %.2f m away", rb_listener_distance(&sim->listener, 0),
               rb_listener_distance(&sim->listener, 1));
    else
        printf("the tailpipe %.2f m away", rb_listener_distance(&sim->listener, 0));
    printf(", the intake's mouth %.2f m\n", rb_listener_distance(&sim->listener, 2));
    printf("  physics at %.0f kHz, the pipes at %.0f kHz", 1e-3 / sim->dt, 1e-3 * rb_pipe_rate(sim));
    if (layers != RB_LAYER_ALL) {
        printf("; only");
        for (int k = 0, first = 1; k < 3; k++)
            if (layers & (1 << k)) {
                printf("%s %s", first ? "" : ",", LAYERS[k]);
                first = 0;
            }
    }
    printf("\n");
    float buf[BLOCK_FRAMES];
    const double fs = cfg.sample_rate;
    if (drive != DRIVE_IDLE) {   /* settle first, held and unrecorded */
        for (long k = 0; k < lround(SOUND_SETTLE * fs / BLOCK_FRAMES); k++)
            rb_render_block(sim, buf, BLOCK_FRAMES);
        if (drive == DRIVE_HOLD && e->result.brake_torque > 0.0) {
            /* Stage 12c: from here on a dyno holds it, as on stage 9's test
             * bed, its crank free: each firing and each cycle moves it a
             * little, as they do a real engine's. */
            rb_dyno_init(&dyno, e, rpm);
            sim->dyno = &dyno;
            printf("  on a dyno at %.0f rpm, throttle %.0f %%, after %.1f s to settle\n", rpm,
                   100.0 * throttle, SOUND_SETTLE);
        } else if (drive == DRIVE_HOLD) {
            printf("  held at %.0f rpm, throttle %.0f %% (too little torque for the dyno), after "
                   "%.1f s to settle\n", rpm, 100.0 * throttle, SOUND_SETTLE);
        } else {
            e->hold_speed = 0;   /* the crank's speed and energy are the held ones: no jump */
        }
    }

    const long long total = llround(seconds * fs);
    const double limited0 = rb_get_telemetry(sim, RB_TLM_LIMITED);   /* settling's aren't heard */
    double peak = 0.0, sumsq = 0.0, lo = INFINITY, hi = 0.0;
    double released = -1.0, on_limiter = -1.0, lifted = -1.0, clutch = -1.0, idling = -1.0;
    double fall_from = -1.0, fall_to = -1.0;   /* s: passing 6000 and 2000 rpm, falling */
    const long cuts0 = e->limiter_cuts;
    long long frames = 0;
    int io_error = 0;
    clock_t c0 = clock();
    while (frames < total) {
        const double t = frames / fs;
        /* the driver */
        if ((drive == DRIVE_IDLE || drive == DRIVE_REV) && e->starter
            && (e->rpm_smooth > 0.75 * d.idle_rpm || t > 3.0)) {
            e->starter = 0;
            released = t;
        }
        if (drive == DRIVE_IDLE && !e->starter) e->load_torque = idle_load;
        if (drive == DRIVE_REV) {
            e->throttle_pos = (t >= REV_BLIP_AT && t < REV_BLIP_AT + REV_BLIP) ? REV_BLIP_THROTTLE
                            : (t >= REV_FLOOR_AT && t < REV_FLOOR_AT + REV_FLOOR) ? 1.0 : 0.0;
            if (on_limiter < 0.0 && e->limiter_cuts > cuts0) on_limiter = t;
        }
        if (drive == DRIVE_SWEEP) {
            if (on_limiter < 0.0 && e->limiter_cuts > cuts0) on_limiter = t;
            if (lifted < 0.0 && on_limiter >= 0.0 && t >= on_limiter + SWEEP_LIMITER) {
                e->throttle_pos = 0.0;
                lifted = t;
            }
            if (clutch < 0.0 && lifted >= 0.0 && t >= lifted + SWEEP_LIFT) {
                /* The clutch goes in: the car's inertia leaves the crank,
                 * which keeps its speed (its kinetic energy is the engine's
                 * own from now on), and so does the road's load. */
                e->desc.inertia = own_inertia;
                e->kinetic = 0.5 * own_inertia * e->omega * e->omega;
                clutch = t;
            }
            e->load_torque = (clutch < 0.0) ? road_load(e->omega, car_inertia) : 0.0;
            if (clutch >= 0.0 && idling < 0.0 && (e->rpm_smooth < d.idle_rpm + 50.0 || t > clutch + SWEEP_FALL))
                idling = t;
            if (idling >= 0.0 && t >= idling + SWEEP_IDLE) break;
        }
        if (lifted >= 0.0 || (drive == DRIVE_REV && t > REV_FLOOR_AT + REV_FLOOR)) {   /* falling */
            if (fall_from < 0.0 && e->rpm < 6000.0) fall_from = t;
            if (fall_to < 0.0 && e->rpm < 2000.0) fall_to = t;
        }

        const int n = (int)(total - frames < BLOCK_FRAMES ? total - frames : BLOCK_FRAMES);
        rb_render_block(sim, buf, n);
        if (wav_write(wav, buf, (size_t)n) != WAV_OK) io_error = 1;
        for (int i = 0; i < n; i++) {
            if (fabs(buf[i]) > peak) peak = fabs(buf[i]);
            sumsq += (double)buf[i] * buf[i];
        }
        frames += n;
        if (e->rpm < lo) lo = e->rpm;
        if (e->rpm > hi) hi = e->rpm;
        if (csv) {
            const double rms = rb_get_telemetry(sim, RB_TLM_OUT_RMS);
            const double pa = (gain > 0.0) ? rms * RB_FULL_SCALE_PA / gain : 0.0;
            double row[8] = { frames / fs, e->rpm, e->throttle_pos, e->spark_cut, e->fuel_cut,
                              rb_get_telemetry(sim, RB_TLM_OUT_PEAK), rms,
                              (pa > 0.0) ? 20.0 * log10(pa / 2e-5) : 0.0 };
            if (csv_row(csv, row) != 0) io_error = 1;
        }
    }
    const double cpu = (double)(clock() - c0) / CLOCKS_PER_SEC;
    const double recorded = frames / fs, rms = frames > 0 ? sqrt(sumsq / frames) : 0.0;

    if (drive == DRIVE_IDLE) {
        if (released >= 0.0) printf("  key to START; the starter lets go at %.2f s\n", released);
        printf("  idling at %.0f rpm at the end (%.0f-%.0f rpm over the whole run)\n", e->result.rpm, lo, hi);
    } else if (drive == DRIVE_SWEEP) {
        printf("  wide open from %.0f rpm, %.1f kg m^2 on the crank and the road's load", SWEEP_FROM, inertia);
        if (on_limiter >= 0.0)
            printf(": on the rev limiter after %.1f s, foot off at %.1f s, clutch in at %.1f s, "
                   "idling at %.1f s\n", on_limiter, lifted, clutch, idling);
        else
            printf(": no rev limiter within %.0f s\n", recorded);
    } else if (drive == DRIVE_REV) {
        printf("  revved in neutral: a blip at %.1f s, floored at %.1f s", REV_BLIP_AT, REV_FLOOR_AT);
        if (on_limiter >= 0.0) printf(", on the rev limiter at %.2f s", on_limiter);
        printf(", foot off at %.1f s; idling at %.0f rpm at the end\n", REV_FLOOR_AT + REV_FLOOR, e->result.rpm);
    }
    if (drive == DRIVE_SWEEP || drive == DRIVE_REV) {
        printf("  the rev limiter cut %ld sparks", e->limiter_cuts - cuts0);
        if (fall_from >= 0.0 && fall_to > fall_from)
            printf("; falling, 6000 to 2000 rpm in %.2f s (%.0f rpm/s)", fall_to - fall_from,
                   4000.0 / (fall_to - fall_from));
        printf("\n");
    }
    const double pa = (gain > 0.0) ? rms * RB_FULL_SCALE_PA / gain : 0.0;
    printf("  %.2f s recorded: peak %.3f (%.1f dBFS), rms %.4f: %.1f dB SPL at the listener\n", recorded,
           peak, peak > 0.0 ? 20.0 * log10(peak) : -INFINITY, rms, pa > 0.0 ? 20.0 * log10(pa / 2e-5) : 0.0);
    printf("  soft-limited samples %.0f, non-finite %.0f\n", rb_get_telemetry(sim, RB_TLM_LIMITED) - limited0,
           rb_get_telemetry(sim, RB_TLM_NONFINITE));
    if (cpu > 0.0) printf("  cpu %.1f s (%.2fx real time)\n", cpu, recorded / cpu);

    if (wav_close(wav) != WAV_OK) io_error = 1;
    if (csv && csv_close(csv) != 0) io_error = 1;
    rb_destroy(sim);
    printf("wrote %s (%s)%s%s\n", out_path, use_float ? "32-bit float" : "16-bit PCM, dithered",
           csv_path ? " and " : "", csv_path ? csv_path : "");
    if (io_error) {
        fprintf(stderr, "rb_cli sound: write error\n");
        return 1;
    }
    return 0;
}

/* rb_cli fuzz: random engines, one after another (native/fuzz.h). */
static int cmd_fuzz(int argc, char **argv)
{
    double sets = 20.0, seconds = 10.0, seed = 1.0, os = 4.0;
    int quality = -1;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--sets"))
            ok = !to_double(take_value(argc, argv, &i), &sets);
        else if (!strcmp(a, "-s") || !strcmp(a, "--seconds"))
            ok = !to_double(take_value(argc, argv, &i), &seconds);
        else if (!strcmp(a, "--seed"))
            ok = !to_double(take_value(argc, argv, &i), &seed);
        else if (!strcmp(a, "-x") || !strcmp(a, "--oversample"))
            ok = !to_double(take_value(argc, argv, &i), &os);
        else if (!strcmp(a, "--quality"))
            ok = (quality = quality_from(take_value(argc, argv, &i))) >= 0;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else
            ok = 0;
        if (!ok) {
            fprintf(stderr, "rb_cli fuzz: bad option or value: %s\n", a);
            return 2;
        }
    }
    if (!(sets >= 1.0 && sets <= 1e6) || !(seconds > 0.0 && seconds <= 600.0)
        || !(seed >= 0.0 && seed < 1e15) || !(os >= 1.0 && os <= RB_MAX_OVERSAMPLE)) {
        fprintf(stderr, "rb_cli fuzz: --sets 1..1e6, --seconds (0, 600], --seed >= 0, "
                        "--oversample 1..%d\n", RB_MAX_OVERSAMPLE);
        return 2;
    }
    int failed = 0;
    printf("%8s  %-9s %4s %4s %-8s %4s %8s %9s %7s %8s %7s\n", "seed", "preset", "cyl", "set", "heard",
           "var", "max rpm", "max bar", "max K", "repairs", "cpu s");
    for (long k = 0; k < (long)sets; k++) {
        const unsigned long long sd = (unsigned long long)seed + (unsigned long long)k;
        rb_fuzz_result r;
        const clock_t c0 = clock();
        const int ok = rb_fuzz_run(sd, seconds, (int)os, quality, &r);
        const double cpu = (double)(clock() - c0) / CLOCKS_PER_SEC;
        printf("%8llu  %-9s %4d %4d %-8s %4.1f %8.0f %9.1f %7.0f %8ld %7.1f  %s\n", sd,
               r.preset >= 0 ? rb_preset_key(r.preset) : "?", r.cylinders, r.changes,
               (r.hear >= 0 && r.hear < RB_HEAR_COUNT) ? LISTENERS[r.hear] : "?", r.variation, r.max_rpm,
               r.max_p / 1e5, r.max_T, r.repairs, cpu, ok ? "ok" : r.what);
        if (!ok)   /* what it changed, to make it again */
            for (int j = 0; j < r.tries; j++) {
                const rb_fuzz_change *c = &r.change[j];
                if (rb_engine_field_flags(c->field) & RB_EF_READONLY) continue;
                printf("            %s[%d] = %g%s\n", rb_engine_field_name(c->field), c->index, c->value,
                       c->rc == RB_OK ? "" : c->rc == RB_CLAMPED ? " (clamped)" : " (refused)");
            }
        fflush(stdout);
        if (!ok) failed++;
    }
    printf("%ld runs of %.1f s: %d failed\n", (long)sets, seconds, failed);
    return failed ? 1 : 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(stderr);
        return 2;
    }
    if (!strcmp(argv[1], "info"))
        return cmd_info();
    if (!strcmp(argv[1], "tone"))
        return cmd_tone(argc - 1, argv + 1);
    if (!strcmp(argv[1], "blowdown"))
        return cmd_blowdown(argc - 1, argv + 1);
    if (!strcmp(argv[1], "motor"))
        return cmd_motor(argc - 1, argv + 1);
    if (!strcmp(argv[1], "breathe"))
        return cmd_breathe(argc - 1, argv + 1);
    if (!strcmp(argv[1], "fire"))
        return cmd_fire(argc - 1, argv + 1);
    if (!strcmp(argv[1], "run"))
        return cmd_run(argc - 1, argv + 1);
    if (!strcmp(argv[1], "presets"))
        return cmd_presets(argc - 1, argv + 1);
    if (!strcmp(argv[1], "dyno"))
        return cmd_dyno(argc - 1, argv + 1);
    if (!strcmp(argv[1], "pipes"))
        return cmd_pipes(argc - 1, argv + 1);
    if (!strcmp(argv[1], "sound"))
        return cmd_sound(argc - 1, argv + 1);
    if (!strcmp(argv[1], "fuzz"))
        return cmd_fuzz(argc - 1, argv + 1);
    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        usage(stdout);
        return 0;
    }
    fprintf(stderr, "rb_cli: unknown command '%s'\n\n", argv[1]);
    usage(stderr);
    return 2;
}
