/* Actual ECU lift/coast/clutch source tracing; diagnostic only.
 * Reuse the frozen, production-identity-checked source separation helpers.
 * This translation unit does not change the accepted renderer or its presets.
 */
#define main held_source_probe_main
#include "audio_source_probe.c"
#undef main

static double coast_road_load(double omega, double car_inertia)
{
    if (!(car_inertia > 0.0)) return 0.0;
    const double ratio = 0.31 * sqrt(1300.0 / car_inertia);
    const double v = omega * 0.31 / ratio;
    return (0.012 * 1300.0 * 9.81 + 0.5 * 1.2 * 0.70 * v * v) * 0.31 / ratio;
}

static const char *const extra_columns[] = {
    "spark_cut", "overrun_time_s", "burn0_spark_angle_deg", "burn0_start_angle_deg",
    "burn0_active", "crank_inertia_kgm2", "load_torque_Nm", "gas_torque_Nm", "limiter_cuts"
};
#define EXTRA_COLS (sizeof extra_columns / sizeof extra_columns[0])
#define COAST_COLS (NCOLS + EXTRA_COLS)

int main(int argc, char **argv)
{
    const char *key = "i6", *directory = NULL;
    double rpm = 0.0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            puts("audio_coast_probe --preset i6|v8-cross|v8-flat --out-dir EXISTING_DIR [--rpm RPM]\n"
                 "192/48/48 kHz engine/pipe/output, gain .1, mono behind-car listener.\n"
                 "1.5 s held loaded settle; then 0.5 s free loaded, 8 s loaded road coast,\n"
                 "clutch-in and 1.5 s recovery. This is an upper-RPM start, not a full pull.\n"
                 "Six simultaneous stems; actual ECU fuel/spark cut, no AFR override.");
            return 0;
        }
        if (i+1 == argc) fail("missing coast option value");
        const char *option = argv[i], *value = argv[++i];
        if (!strcmp(option, "--preset")) key = value;
        else if (!strcmp(option, "--out-dir")) directory = value;
        else if (!strcmp(option, "--rpm")) rpm = strtod(value, NULL);
        else fail("unknown coast option");
    }
    const int preset = rb_preset_find(key);
    if (!rpm) rpm = preset == 1 ? 7500.0 : preset == 2 ? 6500.0 : 8000.0;
    struct stat st;
    if (preset < 1 || preset > 3 || !directory || !isfinite(rpm) || rpm < 1000.0 || rpm > 9000.0
        || stat(directory, &st) || !S_ISDIR(st.st_mode)) fail("invalid coast arguments");
    factor = 4;
    gain = 0.1;
    const double identity = verify_core(preset, rpm, 192000.0, 48000.0);
    init_fixture(preset, rpm, 192000.0, 48000.0);
    const double own_inertia = engine.desc.inertia;
    const double car_inertia = 3.0-own_inertia;
    engine.desc.inertia = 3.0;
    const clock_t start = clock();
    settle(1.5);
    engine.hold_speed = 0;

    char path[2048];
    path_name(path, sizeof path, directory, "coast", "raw.wav");
    wav_writer *raw_file = wav_open(path, 192000, STEMS, WAV_FLOAT32);
    path_name(path, sizeof path, directory, "coast", "filtered.wav");
    wav_writer *filtered_file = wav_open(path, OUTPUT_RATE, STEMS, WAV_FLOAT32);
    path_name(path, sizeof path, directory, "coast", "trace.f64");
    FILE *trace = fopen(path, "wb");
    if (!raw_file || !filtered_file || !trace) fail("coast output open");
    const double lift = 0.5, clutch = 8.5, duration = 10.0;
    double fuel_cut_at = -1.0, fuel_resume_at = -1.0, idle_at = -1.0;
    double raw_error = 0.0, filtered_error = 0.0, full_peak = 0.0;
    long limited = 0;
    float raw_frame[4*STEMS], filtered_frame[STEMS];
    for (long k = 0; k < lround(duration*OUTPUT_RATE); k++) {
        const double t = k/(double)OUTPUT_RATE;
        if (t >= lift) engine.throttle_pos = 0.0;
        if (t >= clutch && engine.desc.inertia > own_inertia) {
            engine.desc.inertia = own_inertia;
            engine.kinetic = 0.5 * own_inertia * engine.omega * engine.omega;
        }
        engine.load_torque = t < clutch ? coast_road_load(engine.omega, car_inertia) : 0.0;
        for (int j = 0; j < factor; j++) {
            physics_step();
            if (fuel_cut_at < 0.0 && engine.fuel_cut) fuel_cut_at = t+(j+1)*engine.dt;
            if (fuel_cut_at >= 0.0 && fuel_resume_at < 0.0 && !engine.fuel_cut)
                fuel_resume_at = t+(j+1)*engine.dt;
            if (t >= clutch && idle_at < 0.0 && engine.rpm_smooth < engine.desc.idle_rpm+50.0)
                idle_at = t+(j+1)*engine.dt;
            double sum = 0.0;
            for (int s = 0; s < STEMS; s++) {
                raw_frame[j*STEMS+s] = (float)raw[s];
                if (s) sum += raw[s];
            }
            raw_error = fmax(raw_error, fabs(raw[0]-sum));
        }
        output_frame();
        double sum = 0.0;
        for (int s = 0; s < STEMS; s++) {
            if (fabs(filtered[s]) > RB_LIMIT_KNEE) limited++;
            filtered_frame[s] = (float)filtered[s];
            if (s) sum += filtered[s];
        }
        filtered_error = fmax(filtered_error, fabs(filtered[0]-sum));
        full_peak = fmax(full_peak, fabs(filtered[0]));
        double row[COAST_COLS];
        trace_values((k+1.0)/OUTPUT_RATE, 0, row);
        const double extra[] = {
            engine.spark_cut, engine.overrun_time, engine.burn[0].spark_angle,
            engine.burn[0].start_angle, engine.burn[0].burning, engine.desc.inertia,
            engine.load_torque, engine.gas_torque, (double)engine.limiter_cuts
        };
        memcpy(row+NCOLS, extra, sizeof extra);
        for (size_t c = 0; c < COAST_COLS; c++) if (!isfinite(row[c])) fail("nonfinite coast trace");
        if (fwrite(row, sizeof(double), COAST_COLS, trace) != COAST_COLS
            || wav_write(raw_file, raw_frame, factor) || wav_write(filtered_file, filtered_frame, 1))
            fail("coast write");
        if (k && k % (2*OUTPUT_RATE) == 0) {
            fprintf(stderr, "%s: %.1f/10 s, rpm %.1f, ECU fuel_cut %d, repairs %ld\n",
                    key, t, engine.rpm, engine.fuel_cut, repairs());
            fflush(stderr);
        }
    }
    if (wav_close(raw_file) || wav_close(filtered_file) || fclose(trace)) fail("coast close");
    if (limited || repairs() || raw_error > 1e-9 || filtered_error > 1e-9
        || fuel_cut_at < lift || fuel_cut_at > lift+0.3) fail("coast diagnostic checks failed");
    if (snprintf(path, sizeof path, "%s/metadata.json", directory) >= (int)sizeof path)
        fail("coast metadata path");
    FILE *meta = fopen(path, "w");
    if (!meta) fail("coast metadata open");
    fprintf(meta, "{\"format_version\":1,\"kind\":\"actual-ECU-upper-start-coast\",\"preset\":\"%s\","
            "\"initial_held_rpm\":%.12g,\"physics_rate_hz\":192000,\"actual_pipe_rate_hz\":%.12g,"
            "\"output_rate_hz\":48000,\"gain\":0.1,\"seconds\":10,\"settle_s\":1.5,"
            "\"listener\":\"mono RB_HEAR_BEHIND, no scene\",\"own_inertia_kgm2\":%.12g,"
            "\"loaded_inertia_kgm2\":3,\"rev_limit\":%.12g,\"production_held_identity_max_abs_error\":%.12g,"
            "\"raw_sum_max_abs_error\":%.12g,\"filtered_sum_max_abs_error\":%.12g,"
            "\"full_peak\":%.12g,\"samples_above_limiter_knee_all_stems\":%ld,\"repairs_total\":%ld,"
            "\"probe_cpu_s\":%.9g,\"final_rpm\":%.12g,\"noise_seed_hex\":\"%llx\","
            "\"markers\":{\"lift_s\":%.12g,\"ecu_fuel_cut_s\":%.12g,\"clutch_s\":%.12g,"
            "\"fuel_resume_s\":%.12g,\"idle_checkpoint_s\":%.12g},"
            "\"phase_windows_s\":{\"last_loaded\":[0.1,0.4],\"ignition_transition\":[0.5,0.65],"
            "\"early_coast\":[0.7,1.3],\"late_coast\":[4.5,6.5],\"pre_clutch\":[7.9,8.4],"
            "\"post_clutch\":[8.7,9.3],\"end_recovery\":[9.5,10]},",
            key, rpm, 1.0/engine.pipe_dt, own_inertia, engine.desc.rev_limit, identity, raw_error,
            filtered_error, full_peak, limited, repairs(), (double)(clock()-start)/CLOCKS_PER_SEC,
            engine.rpm, (unsigned long long)RB_NOISE_SEED, lift, fuel_cut_at, clutch,
            fuel_resume_at, idle_at);
    fputs("\"method\":\"Upper-RPM held loaded start, 1.5 s settle, then free loaded .5 s, "
          "pedal lift and 8 s road coast, clutch-in and first 1.5 s recovery. Not a full pull; "
          "ending does not imply idle is reached. Native CLI road-load equation, driver updated each "
          "48-kHz output frame. Actual ECU retard/fuel-cut/resume, unchanged AFR and ignition. "
          "One engine/all-noise trajectory; six simultaneous disjoint source listeners. Common fixed "
          "gain, no normalization. Raw physics-rate listener pressure precedes FIR/DC; filtered WAV "
          "is 48 kHz after FIR/DC. Limiter is identity at zero count. Trace is instantaneous 48-kHz "
          "native float64, no antialiasing. Identity check compares held source framework to production; "
          "full transient separation is checked for linearity, not a second production-engine rerun.\",", meta);
    fputs("\"stems\":[", meta);
    for (int s = 0; s < STEMS; s++) fprintf(meta, "%s\"%s\"", s ? "," : "", stems[s]);
    fputs("],\"trace_dtype\":\"native float64\",\"trace_columns\":[", meta);
    for (size_t c = 0; c < NCOLS; c++) fprintf(meta, "%s\"%s\"", c ? "," : "", columns[c]);
    for (size_t c = 0; c < EXTRA_COLS; c++) fprintf(meta, ",\"%s\"", extra_columns[c]);
    fputs("],\"pipes\":[", meta);
    int cells = 0;
    for (int i = 0; i < rb_engine_pipe_count(&engine); i++) {
        const rb_pipe *p = rb_engine_pipe(&engine, i); cells += p->cells;
        fprintf(meta, "%s{\"index\":%d,\"cells\":%d,\"dx_m\":%.12g,\"length_m\":%.12g}",
                i ? "," : "", i, p->cells, p->dx, p->length);
    }
    fprintf(meta, "],\"total_cells\":%d}\n", cells);
    if (fclose(meta)) fail("coast metadata close");
    fprintf(stderr, "%s coast complete: CPU %.2f s, fuel cut %.6f s, final rpm %.1f\n",
            key, (double)(clock()-start)/CLOCKS_PER_SEC, fuel_cut_at, engine.rpm);
    return 0;
}
