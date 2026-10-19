/*
 * test_main.c - the test runner.
 *
 *   build/rb_tests            run every test
 *   build/rb_tests wav core   run tests whose name contains "wav" or "core"
 *
 * To add a test: write `void test_something(void)` in a tests/test_*.c
 * file, declare it below and add it to TESTS[].
 */
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- core (tests/test_core.c) ---- */
void test_core_config(void);
void test_core_param_table(void);
void test_core_param_set_get(void);
void test_core_tone_frequency_and_level(void);
void test_core_no_clicks_on_frequency_change(void);
void test_core_level_smoothing(void);
void test_core_block_size_independent_and_reset(void);
void test_core_fuzz_output_bounded(void);
void test_core_render_bad_args(void);
void test_core_host_helpers(void);
/* ---- gas properties (tests/test_gas.c) ---- */
void test_gas_matches_nasa_data(void);
void test_gas_textbook_air(void);
void test_gas_u_and_T_are_inverse(void);
void test_gas_mixing_is_mass_weighted(void);
/* ---- chambers and flow (tests/test_flow.c) ---- */
void test_nozzle_formulas(void);
void test_chamber_init_and_repairs(void);
void test_two_tanks_equalise(void);
void test_blowdown_matches_analytic(void);
void test_blowdown_converges_with_time_step(void);
void test_stiff_equalisation_has_no_overshoot(void);
void test_mixing_tracks_species(void);
void test_filling_heats_the_tank(void);
/* ---- pipes (tests/test_pipe.c) ---- */
void test_pipe_cells(void);
void test_pipe_pulse_travels_at_the_speed_of_sound(void);
void test_pipe_friction(void);
void test_pipe_waves_ride_the_flow(void);
void test_pipe_walls(void);
void test_pipe_conserves_everything(void);
void test_pipe_two_rates_agree(void);
void test_pipe_rings(void);
/* ---- cylinder (tests/test_cylinder.c) ---- */
void test_cylinder_geometry(void);
void test_compression_matches_isentrope(void);
void test_compression_converges_with_step_size(void);
void test_motoring_has_no_energy_drift(void);
void test_volume_step_is_time_symmetric(void);
void test_big_volume_jump_is_split(void);
void test_cylinder_bad_input(void);
/* ---- valves and throttle (tests/test_valve.c, tests/test_throttle.c) ---- */
void test_valve_lift_profile(void);
void test_valve_flow_area(void);
void test_throttle_geometry(void);
void test_throttle_pedal(void);
/* ---- combustion and walls (tests/test_combustion.c) ---- */
void test_fuel_heat(void);
void test_wiebe_burn(void);
void test_burnable_fuel(void);
void test_constant_volume_burn(void);
void test_woschni(void);
void test_motored_loss_angle(void);
/* ---- engine (tests/test_engine.c) ---- */
void test_engine_description(void);
void test_engine_pistons_follow_the_crank(void);
void test_engine_conserves_mass_and_energy(void);
void test_engine_settles_and_balances(void);
void test_engine_cycle_totals_dont_jitter(void);
void test_engine_cylinders_breathe_alike(void);
void test_engine_pipes_two_rates(void);
void test_engine_throttle_response(void);
void test_engine_choked_throttle(void);
void test_engine_pumping_loss(void);
void test_engine_pressures_dont_zigzag(void);
void test_engine_fired_conserves_everything(void);
void test_engine_fired_full_load(void);
void test_engine_spark_advance(void);
void test_engine_mixture_strength(void);
void test_engine_walls_and_residuals(void);
void test_engine_trapped_charge(void);
void test_engine_burn_stops_at_evo(void);
void test_engine_settles_light_load(void);
void test_engine_spark_change_mid_burn(void);
void test_engine_friction(void);
void test_engine_crank_books(void);
void test_engine_cranks(void);
void test_engine_drives_itself(void);
void test_engine_free_matches_held(void);
void test_engine_burns_at_rest(void);
void test_engine_starter_motor(void);
/* ---- layouts, presets and the engine API (tests/test_presets.c) ---- */
void test_presets_table(void);
void test_presets_fire_where_the_layout_says(void);
void test_layout_rules(void);
void test_presets_run_on_their_own(void);
void test_presets_idle_takes_a_load(void);
void test_presets_idle_on_the_spark_alone(void);
void test_api_engine_fields(void);
void test_api_engine_changes(void);
void test_api_cylinder_state(void);
void test_api_pipes(void);
/* ---- the dyno and the calibration (tests/test_dyno.c) ---- */
void test_dyno_holds_a_speed(void);
void test_dyno_sweeps(void);
void test_dyno_only_absorbs(void);
void test_calibration_matches_real_engines(void);
void test_calibration_compression(void);
void test_calibration_cams(void);
void test_calibration_burn_and_spark(void);
void test_calibration_idle_spark(void);
/* ---- native helpers (tests/test_wav.c, tests/test_csv.c) ---- */
void test_wav_pcm16_roundtrip(void);
void test_wav_float32_stereo_roundtrip(void);
void test_wav_pcm16_clipping(void);
void test_wav_rejects_garbage(void);
void test_csv_basic(void);

#define T(fn) { #fn, fn }
static const struct {
    const char *name;
    void (*fn)(void);
} TESTS[] = {
    T(test_core_config),
    T(test_core_param_table),
    T(test_core_param_set_get),
    T(test_core_tone_frequency_and_level),
    T(test_core_no_clicks_on_frequency_change),
    T(test_core_level_smoothing),
    T(test_core_block_size_independent_and_reset),
    T(test_core_fuzz_output_bounded),
    T(test_core_render_bad_args),
    T(test_core_host_helpers),
    T(test_gas_matches_nasa_data),
    T(test_gas_textbook_air),
    T(test_gas_u_and_T_are_inverse),
    T(test_gas_mixing_is_mass_weighted),
    T(test_nozzle_formulas),
    T(test_chamber_init_and_repairs),
    T(test_two_tanks_equalise),
    T(test_blowdown_matches_analytic),
    T(test_blowdown_converges_with_time_step),
    T(test_stiff_equalisation_has_no_overshoot),
    T(test_mixing_tracks_species),
    T(test_filling_heats_the_tank),
    T(test_pipe_cells),
    T(test_pipe_pulse_travels_at_the_speed_of_sound),
    T(test_pipe_friction),
    T(test_pipe_waves_ride_the_flow),
    T(test_pipe_walls),
    T(test_pipe_conserves_everything),
    T(test_pipe_two_rates_agree),
    T(test_pipe_rings),
    T(test_cylinder_geometry),
    T(test_compression_matches_isentrope),
    T(test_compression_converges_with_step_size),
    T(test_motoring_has_no_energy_drift),
    T(test_volume_step_is_time_symmetric),
    T(test_big_volume_jump_is_split),
    T(test_cylinder_bad_input),
    T(test_valve_lift_profile),
    T(test_valve_flow_area),
    T(test_throttle_geometry),
    T(test_throttle_pedal),
    T(test_fuel_heat),
    T(test_wiebe_burn),
    T(test_burnable_fuel),
    T(test_constant_volume_burn),
    T(test_woschni),
    T(test_motored_loss_angle),
    T(test_engine_description),
    T(test_engine_pistons_follow_the_crank),
    T(test_engine_conserves_mass_and_energy),
    T(test_engine_settles_and_balances),
    T(test_engine_cycle_totals_dont_jitter),
    T(test_engine_cylinders_breathe_alike),
    T(test_engine_pipes_two_rates),
    T(test_engine_throttle_response),
    T(test_engine_choked_throttle),
    T(test_engine_pumping_loss),
    T(test_engine_pressures_dont_zigzag),
    T(test_engine_fired_conserves_everything),
    T(test_engine_fired_full_load),
    T(test_engine_spark_advance),
    T(test_engine_mixture_strength),
    T(test_engine_walls_and_residuals),
    T(test_engine_trapped_charge),
    T(test_engine_burn_stops_at_evo),
    T(test_engine_settles_light_load),
    T(test_engine_spark_change_mid_burn),
    T(test_engine_friction),
    T(test_engine_crank_books),
    T(test_engine_cranks),
    T(test_engine_drives_itself),
    T(test_engine_free_matches_held),
    T(test_engine_burns_at_rest),
    T(test_engine_starter_motor),
    T(test_presets_table),
    T(test_presets_fire_where_the_layout_says),
    T(test_layout_rules),
    T(test_presets_run_on_their_own),
    T(test_presets_idle_takes_a_load),
    T(test_presets_idle_on_the_spark_alone),
    T(test_api_engine_fields),
    T(test_api_engine_changes),
    T(test_api_cylinder_state),
    T(test_api_pipes),
    T(test_dyno_holds_a_speed),
    T(test_dyno_sweeps),
    T(test_dyno_only_absorbs),
    T(test_calibration_matches_real_engines),
    T(test_calibration_compression),
    T(test_calibration_cams),
    T(test_calibration_burn_and_spark),
    T(test_calibration_idle_spark),
    T(test_wav_pcm16_roundtrip),
    T(test_wav_float32_stereo_roundtrip),
    T(test_wav_pcm16_clipping),
    T(test_wav_rejects_garbage),
    T(test_csv_basic),
};

static int g_checks;
static int g_failures;

void rbt_check(int ok, const char *expr, const char *file, int line)
{
    g_checks++;
    if (!ok) {
        g_failures++;
        printf("        FAIL %s:%d: %s\n", file, line, expr);
    }
}

void rbt_check_near(double a, double b, double tol,
                    const char *ea, const char *eb, const char *file, int line)
{
    g_checks++;
    if (!(fabs(a - b) <= tol)) {
        g_failures++;
        printf("        FAIL %s:%d: %s = %.10g, %s = %.10g (diff %.3g, tol %.3g)\n",
               file, line, ea, a, eb, b, fabs(a - b), tol);
    }
}

void rbt_check_int(long long a, long long b,
                   const char *ea, const char *eb, const char *file, int line)
{
    g_checks++;
    if (a != b) {
        g_failures++;
        printf("        FAIL %s:%d: %s = %lld, expected %s = %lld\n",
               file, line, ea, a, eb, b);
    }
}

void rbt_tmp_path(char *buf, size_t size, const char *name)
{
    const char *dir = getenv("RBT_TMPDIR");
    if (!dir || !*dir) dir = ".";
    snprintf(buf, size, "%s/%s", dir, name);
}

static int selected(const char *name, int argc, char **argv)
{
    if (argc < 2) return 1;
    for (int i = 1; i < argc; i++)
        if (strstr(name, argv[i])) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    int ran = 0, failed_tests = 0;

    for (size_t i = 0; i < sizeof TESTS / sizeof TESTS[0]; i++) {
        if (!selected(TESTS[i].name, argc, argv)) continue;
        int before = g_failures;
        TESTS[i].fn();
        int ok = (g_failures == before);
        printf("  %s  %s\n", ok ? "pass" : "FAIL", TESTS[i].name);
        fflush(stdout);
        ran++;
        if (!ok) failed_tests++;
    }

    printf("\n%d tests, %d checks: %s\n", ran, g_checks,
           failed_tests ? "FAILED" : "all passed");
    if (failed_tests)
        printf("%d failing test(s), %d failed check(s)\n", failed_tests, g_failures);
    return failed_tests ? 1 : 0;
}
