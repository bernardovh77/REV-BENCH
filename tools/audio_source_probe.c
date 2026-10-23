/* Independent diagnostic only: link to the unchanged core, never production.
 * One engine/noise trajectory supplies every source stem. See --help.
 * Build: cc -O3 -std=c11 -ffp-contract=off -Icore/include -Icore/src -Inative
 * tools/audio_source_probe.c [the core .c files] native/wav.c -lm -o /tmp/audio_source_probe
 */
#include "rb_internal.h"
#include "presets.h"
#include "wav.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define OUTPUT_RATE 48000
#define STEMS 6
static const char *const stems[STEMS] = {
    "full", "pipe_tail", "pipe_intake", "jet_tail", "jet_intake", "mechanical"
};
static const char *const columns[] = {
    "t_s", "theta_deg", "rpm", "throttle", "ignition", "ecu_fuel_cut", "forced_injector_cut",
    "cyl0_p_pa", "cyl0_T_K", "exhaust0_mdot_kg_s", "exhaust0_area_m2", "burn0_heat_W",
    "header0_start_p_pa", "header0_start_T_K", "header0_start_u_m_s",
    "header0_end_p_pa", "header0_end_T_K", "header0_end_u_m_s",
    "collector0_p_pa", "collector0_T_K",
    "tail0_start_p_pa", "tail0_start_T_K", "tail0_start_u_m_s",
    "tail0_end_p_pa", "tail0_end_T_K", "tail0_end_u_m_s",
    "tail0_q_m3_s", "tail1_q_m3_s", "mouth_q_m3_s",
    "source_jet_tail0_pa_1m", "source_jet_tail1_pa_1m", "source_jet_intake_pa_1m",
    "source_mechanical_pa_1m", "map_pa", "fuel_in_kg", "fuel_burned_kg", "heat_released_J"
};
#define NCOLS (sizeof columns / sizeof columns[0])
static rb_engine engine;
static rb_noise noise;
static rb_listener listeners[STEMS];
static rb_decimator aa[STEMS];
static rb_dc_blocker dc[STEMS];
static double raw[STEMS], filtered[STEMS], after_aa[STEMS];
static int factor;
static double gain = 0.1;

static void fail(const char *message)
{
    fprintf(stderr, "audio_source_probe: %s (%s)\n", message, strerror(errno));
    exit(1);
}

static void selected_flows(int stem, double q[3])
{
    q[0] = (stem == 0 || stem == 1) ? engine.tail_flow[0] : 0.0;
    q[1] = (stem == 0 || stem == 1) ? engine.tail_flow[1] : 0.0;
    q[2] = (stem == 0 || stem == 2) ? engine.mouth_flow : 0.0;
}

static void init_fixture(int preset, double rpm, double rate, double pipes)
{
    rb_engine_desc d;
    if (rb_preset_desc(preset, &d) || rb_engine_init_at(&engine, &d, 1.0/rate, pipes))
        fail("cannot initialize engine");
    engine.ignition = 1;
    engine.variation = 1.0;
    engine.rpm = rpm;
    engine.throttle_pos = 1.0;
    rb_noise_init(&noise, &engine, RB_NOISE_SEED);
    const double sound = rb_chamber_sound_speed(&engine.exhaust_air);
    for (int s = 0; s < STEMS; s++) {
        rb_listener_place(&listeners[s], RB_HEAR_BEHIND, engine.systems, sound, engine.pipe_dt);
        rb_listener_place_noise(&listeners[s], RB_HEAR_BEHIND, engine.systems, sound,
                                engine.pipe_dt, engine.dt);
        double q[3]; selected_flows(s, q);
        rb_listener_prime(&listeners[s], q);
        if (rb_decimator_init(&aa[s], factor)) fail("unsupported decimator factor");
        rb_dc_blocker_init(&dc[s], RB_DC_CUTOFF, OUTPUT_RATE);
    }
}

/* The engine and ALL noise are evaluated once. Independent listener memories
 * select disjoint physical source locations; no layer-dependent RNG reruns. */
static void physics_step(void)
{
    const long previous = engine.pipe_steps;
    rb_engine_step(&engine);
    rb_noise_step(&noise, &engine, RB_NOISE_FLOW | RB_NOISE_ENGINE);
    for (int s = 0; s < STEMS; s++) {
        if (engine.pipe_steps != previous) {
            double q[3]; selected_flows(s, q);
            rb_listener_push(&listeners[s], q);
        }
        double p[RB_LISTEN_NOISES] = {0};
        if (s == 0 || s == 3) { p[0] = noise.out[0]; p[1] = noise.out[1]; }
        if (s == 0 || s == 4) p[2] = noise.out[2];
        if (s == 0 || s == 5) p[3] = noise.out[3];
        rb_listener_push_noise(&listeners[s], p, (s == 0 || s == 5) ? noise.out[4] : 0.0);
        raw[s] = rb_listener_hear(&listeners[s], engine.exhaust_air.rho, engine.dt)
                 / RB_FULL_SCALE_PA * gain;
        if (!isfinite(raw[s]) || fabs(raw[s]) > RB_MIC_MAX * gain)
            fail("nonfinite or safety-clamped raw source; refusing capture");
        rb_decimator_push(&aa[s], raw[s]);
    }
}

static void output_frame(void)
{
    for (int s = 0; s < STEMS; s++) {
        after_aa[s] = rb_decimator_out(&aa[s]);
        filtered[s] = rb_dc_blocker_step(&dc[s], after_aa[s]);
    }
}

static void settle(double seconds)
{
    for (long k = 0; k < lround(seconds*OUTPUT_RATE); k++) {
        for (int j = 0; j < factor; j++) physics_step();
        output_frame();
    }
}

static long repairs(void)
{
    long total = engine.airbox.repairs + engine.plenum.repairs + engine.junction.repairs;
    for (int i = 0; i < engine.desc.cylinders; i++) total += engine.cyl[i].gas.repairs;
    for (int i = 0; i < engine.collectors; i++) total += engine.collector[i].repairs;
    for (int i = 0; i < rb_engine_pipe_count(&engine); i++) {
        const rb_pipe *p = rb_engine_pipe(&engine, i);
        for (int j = 0; j < p->cells; j++) total += p->cell[j].repairs;
    }
    return total;
}

static void trace_values(double t, int cut, double row[NCOLS])
{
    const rb_pipe *h = &engine.header[0], *tail = &engine.tailpipe[0];
    const rb_chamber *ha = &h->cell[0], *hb = &h->cell[h->cells-1];
    const rb_chamber *ta = &tail->cell[0], *tb = &tail->cell[tail->cells-1];
    const double values[] = {
        t, engine.theta, engine.rpm, engine.throttle_pos, engine.ignition, engine.fuel_cut, cut,
        engine.cyl[0].gas.p, engine.cyl[0].gas.T, engine.exhaust[0].mdot,
        engine.exhaust[0].area, engine.burn[0].heat/engine.dt,
        ha->p, ha->T, rb_pipe_cell_velocity(h, 0),
        hb->p, hb->T, rb_pipe_cell_velocity(h, h->cells-1),
        engine.collector[0].p, engine.collector[0].T,
        ta->p, ta->T, rb_pipe_cell_velocity(tail, 0),
        tb->p, tb->T, rb_pipe_cell_velocity(tail, tail->cells-1),
        engine.tail_flow[0], engine.tail_flow[1], engine.mouth_flow,
        noise.out[0], noise.out[1], noise.out[2], noise.out[3], engine.plenum.p,
        engine.fuel_in, engine.fuel_burned, engine.heat_released
    };
    _Static_assert(sizeof values / sizeof values[0] == NCOLS, "trace column count");
    memcpy(row, values, sizeof values);
}

static void path_name(char *path, size_t n, const char *directory, const char *state, const char *suffix)
{
    if (snprintf(path, n, "%s/%s-%s", directory, state, suffix) >= (int)n)
        fail("output path too long");
}

static double capture(const char *directory, const char *state, double seconds, int cut, FILE *meta)
{
    char path[2048];
    path_name(path, sizeof path, directory, state, "raw.wav");
    wav_writer *raw_file = wav_open(path, OUTPUT_RATE*factor, STEMS, WAV_FLOAT32);
    path_name(path, sizeof path, directory, state, "filtered.wav");
    wav_writer *filtered_file = wav_open(path, OUTPUT_RATE, STEMS, WAV_FLOAT32);
    path_name(path, sizeof path, directory, state, "after-aa.wav");
    wav_writer *aa_file = wav_open(path, OUTPUT_RATE, STEMS, WAV_FLOAT32);
    path_name(path, sizeof path, directory, state, "trace.f64");
    FILE *trace = fopen(path, "wb");
    if (!raw_file || !filtered_file || !aa_file || !trace) fail("cannot open capture files");
    float raw_block[32*STEMS], block[STEMS], aa_block[STEMS];
    double raw_error = 0.0, filtered_error = 0.0, peak = 0.0;
    long limited = 0, samples = lround(seconds*OUTPUT_RATE);
    const long old_repairs = repairs();
    const double old_fuel = engine.fuel_in, old_burned = engine.fuel_burned;
    const double old_heat = engine.heat_released;
    const clock_t start = clock();
    for (long k = 0; k < samples; k++) {
        for (int j = 0; j < factor; j++) {
            physics_step();
            double sum = 0.0;
            for (int s = 0; s < STEMS; s++) {
                raw_block[j*STEMS+s] = (float)raw[s];
                if (s) sum += raw[s];
            }
            raw_error = fmax(raw_error, fabs(raw[0]-sum));
        }
        output_frame();
        double sum = 0.0;
        for (int s = 0; s < STEMS; s++) {
            if (fabs(filtered[s]) > RB_LIMIT_KNEE) limited++;
            /* Keep every stem linear. The production limiter would be the
             * identity if limited==0; refuse interpretation otherwise. */
            block[s] = (float)filtered[s];
            aa_block[s] = (float)after_aa[s];
            if (s) sum += filtered[s];
        }
        filtered_error = fmax(filtered_error, fabs(filtered[0]-sum));
        peak = fmax(peak, fabs(filtered[0]));
        double row[NCOLS]; trace_values((k+1.0)/OUTPUT_RATE, cut, row);
        for (size_t c = 0; c < NCOLS; c++) if (!isfinite(row[c])) fail("nonfinite trace");
        if (fwrite(row, sizeof(double), NCOLS, trace) != NCOLS
            || wav_write(raw_file, raw_block, factor)
            || wav_write(filtered_file, block, 1) || wav_write(aa_file, aa_block, 1)) fail("capture write");
    }
    const double cpu = (double)(clock()-start)/CLOCKS_PER_SEC;
    if (wav_close(raw_file) || wav_close(filtered_file) || wav_close(aa_file) || fclose(trace))
        fail("capture close");
    fprintf(meta, "\"%s\":{\"seconds\":%.12g,\"capture_cpu_s\":%.9g,"
            "\"full_peak\":%.12g,\"raw_sum_max_abs_error\":%.12g,\"filtered_sum_max_abs_error\":%.12g,"
            "\"samples_above_limiter_knee_all_stems\":%ld,\"repairs_capture\":%ld,\"repairs_total\":%ld,"
            "\"injected_fuel_kg\":%.12g,\"burned_fuel_kg\":%.12g,\"heat_released_J\":%.12g,"
            "\"ecu_fuel_cut\":%d,\"forced_injector_cut\":%d,\"brake_torque_Nm_last_cycle\":%.12g}",
            state, samples/(double)OUTPUT_RATE, cpu, peak, raw_error, filtered_error,
            limited, repairs()-old_repairs, repairs(), engine.fuel_in-old_fuel,
            engine.fuel_burned-old_burned, engine.heat_released-old_heat,
            engine.fuel_cut, cut, engine.result.brake_torque);
    fflush(meta);
    fprintf(stderr, "%s: %.3g s capture, %.2f CPU s, peak %.4g, sum error %.3g, repairs %ld\n",
            state, samples/(double)OUTPUT_RATE, cpu, peak, filtered_error, repairs());
    if (limited || repairs() || raw_error > 1e-9 || filtered_error > 1e-9)
        fail("source linearity/limiter/repair diagnostic failed");
    if (cut && engine.fuel_in != old_fuel)
        fail("held injector-cut capture still injects fuel");
    return cpu;
}

static double verify_core(int preset, double rpm, double physics, double pipes)
{
    rb_config cfg = {OUTPUT_RATE, factor, pipes};
    rb_sim *sim = rb_create(&cfg);
    if (!sim || rb_load_preset(sim, preset)) fail("core identity fixture init");
    sim->source = RB_SOURCE_ENGINE;
    sim->engine.ignition = 1;
    sim->engine.variation = 1.0;
    sim->engine.rpm = rpm;
    sim->engine.throttle_pos = 1.0;
    sim->gain_s = sim->param[RB_PARAM_MASTER_GAIN] = gain;
    rb_sim_listen(sim);
    init_fixture(preset, rpm, physics, pipes);
    double error = 0.0;
    for (int k = 0; k < 4096; k++) {
        float actual;
        rb_render_block(sim, &actual, 1);
        for (int j = 0; j < factor; j++) physics_step();
        output_frame();
        error = fmax(error, fabs(actual-(float)rb_soft_limit(filtered[0])));
    }
    rb_destroy(sim);
    if (error > 2e-7) fail("independent full stem differs from production rb_render_block");
    return error;
}

int main(int argc, char **argv)
{
    const char *key = "i6", *directory = NULL;
    double rpm = 3000.0, physics = 192000.0, pipes = 48000.0, warm = 1.5, seconds = 0.75;
    int check_core = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            puts("audio_source_probe --preset i6|v8-cross|v8-flat --rpm RPM --out-dir EXISTING_DIR\n"
                 " [--physics-rate 192000] [--pipe-rate 48000] [--settle 1.5] [--seconds .75]\n"
                 " [--gain .1] [--verify-core]\n"
                 "Captures loaded then throttle-closed/injection-disabled at exactly held RPM.\n"
                 "All stems share ONE engine/noise trajectory; no scene, mono behind-car listener.\n"
                 "Raw physics-rate / after-AA / filtered float WAVs have six channels listed in metadata.\n"
                 "Native-endian float64 traces are instantaneous 48 kHz samples, no antialiasing.\n"
                 "Injection cut uses afr=0 because held engine clears ECU fuel_cut; not a road coast.");
            return 0;
        }
        if (!strcmp(argv[i], "--verify-core")) { check_core = 1; continue; }
        if (i+1 == argc) fail("missing option value");
        const char *option = argv[i], *value = argv[++i];
        if (!strcmp(option, "--preset")) key = value;
        else if (!strcmp(option, "--out-dir")) directory = value;
        else if (!strcmp(option, "--rpm")) rpm = strtod(value, NULL);
        else if (!strcmp(option, "--physics-rate")) physics = strtod(value, NULL);
        else if (!strcmp(option, "--pipe-rate")) pipes = strtod(value, NULL);
        else if (!strcmp(option, "--settle")) warm = strtod(value, NULL);
        else if (!strcmp(option, "--seconds")) seconds = strtod(value, NULL);
        else if (!strcmp(option, "--gain")) gain = strtod(value, NULL);
        else fail("unknown option");
    }
    const int preset = rb_preset_find(key);
    factor = (int)lround(physics/OUTPUT_RATE);
    if (preset < 0 || !directory || !isfinite(physics) || !isfinite(rpm) || !isfinite(pipes)
        || !isfinite(seconds) || !isfinite(warm) || !isfinite(gain) || factor < 1 || factor > 32
        || fabs(factor*OUTPUT_RATE-physics) > 1e-6 || rpm < 500 || rpm > 15000
        || pipes < 12000 || pipes > 96000 || seconds < .1 || seconds > 10 || warm < .1 || warm > 20
        || gain <= 0 || gain > 1) fail("invalid fixture arguments (see --help)");
    struct stat st;
    if (stat(directory, &st) || !S_ISDIR(st.st_mode)) fail("--out-dir must already exist");
    const clock_t start = clock();
    const double identity = check_core ? verify_core(preset, rpm, physics, pipes) : -1.0;
    init_fixture(preset, rpm, physics, pipes);
    char path[2048];
    if (snprintf(path, sizeof path, "%s/metadata.json", directory) >= (int)sizeof path)
        fail("output path too long");
    FILE *meta = fopen(path, "w");
    if (!meta) fail("metadata open");
    fprintf(meta, "{\n\"format_version\":1,\"preset\":\"%s\",\"rpm\":%.12g,"
            "\"physics_rate_hz\":%.12g,\"requested_pipe_rate_hz\":%.12g,\"actual_pipe_rate_hz\":%.12g,"
            "\"output_rate_hz\":%d,\"gain\":%.12g,\"full_scale_pa\":%.12g,\"settle_each_state_s\":%.12g,"
            "\"listener\":\"mono RB_HEAR_BEHIND, no scene\",\"variation\":1,"
            "\"rev_limit\":%.12g,\"noise_seed_hex\":\"%llx\",\"production_core_max_abs_error\":%.12g,"
            "\"method\":\"One engine and all-noise trajectory. Disjoint source listeners; common fixed gain. "
            "Raw WAV is physics-rate listener pressure before FIR/DC; after-aa is FIR/decimation only; "
            "filtered is after 10-Hz DC blocker. Limiter is identity only when count is zero. "
            "Cut state closes throttle and sets afr=0 to disable injectors while held, retains ignition; "
            "held mode clears ECU fuel_cut. Residual fuel may still burn and is measured. "
            "Noise injector clicks remain enabled by ignition&&!fuel_cut despite afr=0; the mechanical "
            "stem is not exact ECU fuel-cut audio. This is not road overrun. Trace samples are instantaneous "
            "at 48 kHz without antialias filtering; pressure/flow ratios are not isolated transfer functions.\",\n",
            key, rpm, physics, pipes, 1.0/engine.pipe_dt, OUTPUT_RATE, gain, RB_FULL_SCALE_PA,
            warm, engine.desc.rev_limit, (unsigned long long)RB_NOISE_SEED, identity);
    fputs("\"stems\":[", meta);
    for (int s = 0; s < STEMS; s++) fprintf(meta, "%s\"%s\"", s ? "," : "", stems[s]);
    fputs("],\"trace_dtype\":\"native float64\",\"trace_columns\":[", meta);
    for (size_t c = 0; c < NCOLS; c++) fprintf(meta, "%s\"%s\"", c ? "," : "", columns[c]);
    fputs("],\"pipes\":[", meta);
    int cells = 0;
    for (int i = 0; i < rb_engine_pipe_count(&engine); i++) {
        const rb_pipe *p = rb_engine_pipe(&engine, i);
        cells += p->cells;
        fprintf(meta, "%s{\"index\":%d,\"cells\":%d,\"dx_m\":%.12g,\"length_m\":%.12g,"
                "\"diameter_m\":%.12g,\"momentum_order\":%d}", i ? "," : "", i,
                p->cells, p->dx, p->length, p->diameter, p->momentum_order);
    }
    fprintf(meta, "],\"total_cells\":%d,\"captures\":{", cells);
    settle(warm);
    capture(directory, "loaded", seconds, 0, meta);
    fputs(",\n", meta);
    engine.throttle_pos = 0.0;
    engine.afr = 0.0;
    settle(warm);
    capture(directory, "cut", seconds, 1, meta);
    fprintf(meta, "},\"total_cpu_s\":%.9g}\n", (double)(clock()-start)/CLOCKS_PER_SEC);
    if (fclose(meta)) fail("metadata close");
    return 0;
}
