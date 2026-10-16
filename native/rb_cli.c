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
 *
 * The native tools may use the core's internal headers (chamber.h, ...) to
 * exercise pieces of the physics on their own.
 */
#include "chamber.h"
#include "combustion.h"
#include "csv.h"
#include "cylinder.h"
#include "engine.h"
#include "flow.h"
#include "rb.h"
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
        "  info                  list parameters and telemetry channels\n"
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
        "\n"
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
        "breathe options (the default engine is a 2.0 L inline four):\n"
        "      --rpm N           crank speed                (default 3000)\n"
        "      --throttle X      throttle opening, 0..1     (default 1)\n"
        "      --cylinders N     1..16, firing evenly       (default 4, firing order 1-3-4-2)\n"
        "      --plenum L        plenum volume in liters    (default 2.5)\n"
        "      --intake D/L/C    intake cam: duration (deg), lift (mm), centerline (deg after\n"
        "                        overlap TDC)               (default 250/9.5/110)\n"
        "      --exhaust D/L/C   exhaust cam: same, centerline in deg before overlap TDC\n"
        "                                                   (default 250/9/110)\n"
        "      --csv FILE        one cycle of cylinder 1 and the plenum\n"
        "                                                   (default renders/breathe.csv)\n"
        "      --sweep           measure a whole map instead: rpm 800-7000 x 9 throttle\n"
        "                        openings, written to the CSV (default renders/ve.csv)\n"
        "      --walls           heat flow to the cylinder walls (default: adiabatic, as in stage 5)\n"
        "\n",
        f);
    fputs(
        "fire options (also --rpm, --throttle, --cylinders, --plenum, --intake, --exhaust):\n"
        "      --spark DEG       spark advance, degrees before TDC (default 25)\n"
        "      --afr R           air-fuel ratio by mass     (default 14.6, stoichiometric)\n"
        "      --motored         ignition off, for comparison\n"
        "      --no-walls        no heat flow to the walls\n"
        "      --csv FILE        one cycle of cylinder 1    (default renders/fire.csv)\n"
        "      --spark-sweep     torque and peak pressure for spark 0-50 deg (default renders/spark.csv)\n"
        "      --rpm-sweep       wide-open torque and power, 1000-7000 rpm (default renders/power.csv)\n"
        "\n"
        "run options (15 s: start, idle, a blip at 5 s, floored at 8.5 s, off at 13 s):\n"
        "      --csv FILE        every millisecond          (default renders/run.csv)\n"
        "      --load N          torque the crank drives, N m (default 0: in neutral)\n"
        "      --inertia KGM2    crank, flywheel and clutch (default 0.16)\n"
        "      --idle RPM        idle speed                 (default 800)\n"
        "      --limit RPM       rev limit                  (default 6800)\n"
        "      --cylinders N     1..16, firing evenly       (default 4, firing order 1-3-4-2)\n",
        f);
}

/* Returns the value that follows option argv[*i], or NULL if missing. */
static const char *take_value(int argc, char **argv, int *i)
{
    if (*i + 1 >= argc) return NULL;
    *i += 1;
    return argv[*i];
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
    printf("wrote %s (%s)%s%s\n", out_path, use_float ? "32-bit float" : "16-bit PCM",
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
        const int choked = atmosphere.p / p <= rb_nozzle_critical_ratio(tank.gamma);
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
                              c.gas.p / 1e5, c.gas.T, c.gas.gamma };
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

static rb_engine engine;   /* about 10 KB: not on the stack */

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

/* One operating point: what the dyno and the driver set. */
typedef struct op_point {
    double rpm, throttle, spark, afr;
    int    ignition;
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
                               g->V * 1e6, g->p / 1e5, g->T, g->gamma,
                               rb_valve_lift(&d->intake, a) * 1e3, rb_valve_lift(&d->exhaust, a) * 1e3,
                               engine.intake[0].mdot * 1e3, engine.exhaust[0].mdot * 1e3,
                               engine.plenum.p / 1e3, engine.rpm, engine.throttle_pos,
                               /* 0 until this cycle's spark: before it the burn record
                                * still holds the last cycle's 1.0 */
                               start + n * step_deg + engine.spark >= 0.0 ? b->fraction : 0.0,
                               b->heat / step_deg, b->wall_heat / step_deg,
                               engine.spark, engine.ignition };
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
            const op_point op = { rpms[i], throttles[j], 25.0, RB_AFR_STOICH, 0 };
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
    double rpm = 3000.0, throttle = 1.0, cylinders = 4.0, plenum = -1.0;
    int do_sweep = 0;

    rb_engine_desc_default(&d);
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
    if (cylinders != floor(cylinders) || cylinders < 1.0 || cylinders > RB_MAX_CYLINDERS) {
        fprintf(stderr, "rb_cli breathe: --cylinders must be a whole number in [1, %d]\n",
                RB_MAX_CYLINDERS);
        return 2;
    }
    if ((int)cylinders != d.cylinders) {   /* evenly spaced, in cylinder order */
        d.cylinders = (int)cylinders;
        for (int i = 0; i < d.cylinders; i++) d.firing[i] = i * 720.0 / d.cylinders;
    }
    if (plenum >= 0.0) d.plenum_volume = plenum * 1e-3;
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli breathe: impossible engine (check the cams: duration 1..360 deg, "
                        "lift below the valve diameter, centerline within the cycle; plenum > 0)\n");
        return 2;
    }

    if (do_sweep) return sweep(&d, csv_path ? csv_path : "renders/ve.csv");

    rb_engine_init(&engine, &d, 1.0 / 192000.0);
    const double litres = rb_engine_displacement(&engine) * 1e3;
    printf("breathe: %d cylinder%s, %.2f L (%.1f x %.1f mm), turned by a motor at %.0f rpm, "
           "throttle %.0f %%, %s walls\n", d.cylinders, d.cylinders > 1 ? "s" : "", litres,
           d.bore * 1e3, d.stroke * 1e3, rpm, throttle * 100.0,
           d.heat_transfer ? "warm" : "adiabatic");
    print_cam("intake", &d.intake, 1);
    print_cam("exhaust", &d.exhaust, 0);
    const double overlap = rb_valve_closes(&d.exhaust) - rb_valve_opens(&d.intake);
    if (overlap > 0.0)
        printf("  overlap  %.0f deg around TDC, with both valves open\n", overlap);
    else
        printf("  no overlap: the exhaust closes %.0f deg before the intake opens\n", -overlap);

    const op_point op = { rpm, throttle, 25.0, RB_AFR_STOICH, 0 };
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
    printf("  volumetric efficiency  %.3f        plenum %.1f kPa, exhaust manifold %.1f kPa\n",
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
                           "brake_torque_Nm", "brake_power_kW" };
    csv_writer *csv = csv_open(path, cols, 19);
    if (!csv) {
        fprintf(stderr, "rb_cli fire: cannot write %s (does the folder exist?)\n", path);
        return 1;
    }
    const double from = by_spark ? 0.0 : 1000.0, to = by_spark ? 50.0 : 7000.0;
    const double step = by_spark ? 2.5 : 500.0;
    double best = -1e300, best_at = 0.0;

    printf("%s, %s\n", by_spark ? "spark sweep" : "wide-open rpm sweep",
           by_spark ? "torque against spark advance" : "indicated torque and power");
    printf("  %8s %9s %9s %9s %10s %9s %10s\n", by_spark ? "spark" : "rpm", "torque", "power", "IMEP",
           "peak p", "at", "brake");
    for (double x = from; x <= to + 1e-9; x += step) {
        if (by_spark) op.spark = x;
        else op.rpm = x;
        const int cycles = settle_at(d, &op);
        const rb_engine_cycle *r = &engine.result;
        double row[19] = { op.rpm, op.throttle, op.spark, op.afr, r->imep_net / 1e5,
                           r->imep_gross / 1e5, r->pmep / 1e5, r->torque, r->power / 1e3,
                           r->efficiency, r->p_max / 1e5, r->angle_p_max, r->ve, r->t_exhaust,
                           cycles, r->fmep / 1e5, r->bmep / 1e5, r->brake_torque,
                           r->brake_power / 1e3 };
        csv_row(csv, row);
        printf("  %8.1f %7.1f Nm %6.1f kW %5.2f bar %6.1f bar %5.1f deg %6.1f Nm\n", x, r->torque,
               r->power / 1e3, r->imep_net / 1e5, r->p_max / 1e5, r->angle_p_max, r->brake_torque);
        if (r->torque > best) {
            best = r->torque;
            best_at = x;
        }
    }
    if (by_spark)
        printf("best torque (MBT) at %.1f deg before TDC\n", best_at);
    else
        printf("peak torque at %.0f rpm\n", best_at);
    printf("wrote %s\n", path);
    return csv_close(csv) ? 1 : 0;
}

static int cmd_fire(int argc, char **argv)
{
    rb_engine_desc d;
    const char *csv_path = NULL;
    double cylinders = 4.0, plenum = -1.0;
    op_point op = { 3000.0, 1.0, 25.0, 14.6, 1 };
    int sweep_spark = 0, sweep_rpm = 0;

    rb_engine_desc_default(&d);
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int ok = 1;
        if (!strcmp(a, "--csv"))
            ok = (csv_path = take_value(argc, argv, &i)) != NULL;
        else if (!strcmp(a, "--rpm"))
            ok = !to_double(take_value(argc, argv, &i), &op.rpm);
        else if (!strcmp(a, "--throttle"))
            ok = !to_double(take_value(argc, argv, &i), &op.throttle);
        else if (!strcmp(a, "--spark"))
            ok = !to_double(take_value(argc, argv, &i), &op.spark);
        else if (!strcmp(a, "--afr"))
            ok = !to_double(take_value(argc, argv, &i), &op.afr);
        else if (!strcmp(a, "--cylinders"))
            ok = !to_double(take_value(argc, argv, &i), &cylinders);
        else if (!strcmp(a, "--plenum"))
            ok = !to_double(take_value(argc, argv, &i), &plenum);
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
    if (cylinders != floor(cylinders) || cylinders < 1.0 || cylinders > RB_MAX_CYLINDERS) {
        fprintf(stderr, "rb_cli fire: --cylinders must be a whole number in [1, %d]\n",
                RB_MAX_CYLINDERS);
        return 2;
    }
    if ((int)cylinders != d.cylinders) {
        d.cylinders = (int)cylinders;
        for (int i = 0; i < d.cylinders; i++) d.firing[i] = i * 720.0 / d.cylinders;
    }
    if (plenum >= 0.0) d.plenum_volume = plenum * 1e-3;
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli fire: impossible engine (check the cams and the plenum)\n");
        return 2;
    }
    if (sweep_spark) return fire_sweep(&d, op, 1, csv_path ? csv_path : "renders/spark.csv");
    if (sweep_rpm) {
        op.throttle = 1.0;
        return fire_sweep(&d, op, 0, csv_path ? csv_path : "renders/power.csv");
    }

    rb_engine_init(&engine, &d, 1.0 / 192000.0);
    printf("fire: %d cylinder%s, %.2f L, held at %.0f rpm by a dyno, throttle %.0f %%\n",
           d.cylinders, d.cylinders > 1 ? "s" : "", rb_engine_displacement(&engine) * 1e3,
           op.rpm, op.throttle * 100.0);
    if (op.ignition)
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
    double load = 0.0, cylinders = 4.0, inertia = -1.0, idle = -1.0, limit = -1.0;

    rb_engine_desc_default(&d);
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
        else if (!strcmp(a, "--cylinders"))
            ok = !to_double(take_value(argc, argv, &i), &cylinders);
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
    if (cylinders != floor(cylinders) || cylinders < 1.0 || cylinders > RB_MAX_CYLINDERS) {
        fprintf(stderr, "rb_cli run: --cylinders must be a whole number in [1, %d]\n",
                RB_MAX_CYLINDERS);
        return 2;
    }
    if ((int)cylinders != d.cylinders) {
        d.cylinders = (int)cylinders;
        for (int i = 0; i < d.cylinders; i++) d.firing[i] = i * 720.0 / d.cylinders;
    }
    if (inertia >= 0.0) d.inertia = inertia;
    if (idle >= 0.0) d.idle_rpm = idle;
    if (limit >= 0.0) {
        d.rev_limit = limit;
        d.rev_limit_resume = limit - 100.0;
    }
    if (rb_engine_desc_check(&d) != 0) {
        fprintf(stderr, "rb_cli run: impossible engine (check --inertia, --idle and --limit)\n");
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
    printf("run: %d cylinder%s, %.2f L, crank free (%.2f kg m^2), idle %.0f rpm, rev limit %.0f rpm",
           d.cylinders, d.cylinders > 1 ? "s" : "", rb_engine_displacement(e) * 1e3, d.inertia,
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
    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        usage(stdout);
        return 0;
    }
    fprintf(stderr, "rb_cli: unknown command '%s'\n\n", argv[1]);
    usage(stderr);
    return 2;
}
