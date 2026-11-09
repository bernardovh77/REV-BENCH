/*
 * host_tests_main.c - the host's test runner (stage 123).
 *
 *   rb_host_tests                run every test
 *   rb_host_tests ring queue     run tests whose name contains "ring" or "queue"
 *   rb_host_tests --list         the names
 *
 * To add a test: write `void test_xxx(void)` in a host/tests/test_*.c file,
 * declare it below and add it to TESTS[].
 */
#include "ht.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- the channels (test_channels.c) ---- */
void test_ring_fifo_wrap_and_counts(void);
void test_ring_rejects_bad_sizes_and_flushes(void);
void test_ring_stress_in_order_no_loss(void);
void test_queue_fifo_refuses_when_full_and_wraps(void);
void test_queue_rejects_bad_sizes(void);
void test_queue_stress_no_loss_no_tear(void);
void test_triple_newest_wins_and_nothing_stale(void);
void test_triple_stress_never_torn(void);
void test_evring_orders_and_counts_losses(void);
void test_evring_stress_taken_plus_lost_is_everything(void);

/* ---- the host (test_host.c) ---- */
void test_host_render_thread_fills_the_ring_to_its_target(void);
void test_host_render_thread_priority_is_set_and_reported(void);
void test_host_command_is_applied_and_acknowledged(void);
void test_host_acks_come_in_order_with_their_ids(void);
void test_host_full_command_queue_refuses_and_never_waits(void);
void test_host_refused_commands_say_why(void);
void test_host_every_command_kind_reaches_the_engine(void);
void test_host_no_command_is_lost_under_load(void);
void test_host_snapshots_are_whole_and_published_about_60_times_a_second(void);
void test_host_a_slow_reader_gets_the_newest_and_drops_are_counted(void);
void test_host_events_arrive_in_order_and_losses_are_the_gaps(void);
void test_host_clock_maps_physics_ticks_to_audio_frames(void);
void test_host_44100_renders_natively(void);
void test_host_output_time_needs_a_device_callback_in_the_epoch(void);
void test_host_underrun_fades_out_in_2ms_then_silence_then_fades_in(void);
void test_host_short_data_before_an_underrun_still_fades_to_zero(void);
void test_audit_counts_forbidden_calls_only_on_the_render_thread(void);
void test_render_thread_never_allocates_locks_or_logs(void);
#ifdef RB_HOST_MALLOC_WRAP
void test_render_thread_never_calls_the_c_libraries_allocator(void);
#endif
void test_host_deadline_bins_are_tenths_then_missed(void);
void test_host_statistics_deadline_histogram_and_ring_fill(void);

/* ---- swaps (test_swap.c) ---- */
void test_swap_installs_the_new_engine_and_frees_the_old_one(void);
void test_swap_fades_without_a_click(void);
void test_swap_failed_preparation_leaves_the_running_engine_untouched(void);
void test_swap_prepare_table_is_bounded_and_unknown_ids_are_refused(void);
void test_swap_stale_generation_is_refused_and_its_engine_freed(void);
void test_swap_rapid_successive_swaps_each_get_their_own_fade_and_ack(void);
void test_swap_busy_when_engines_are_not_being_freed_then_retried(void);
void test_swap_commands_behind_it_wait_and_apply_to_the_new_engine(void);
void test_swap_concurrent_with_the_device_thread_and_readers(void);

/* ---- the offline mode (test_offline.c) and traces (test_trace.c) ---- */
void test_offline_mono_drive_is_the_cores_own_bit_for_bit(void);
void test_offline_stereo_scene_drive_is_the_cores_own_bit_for_bit(void);
void test_offline_publishing_snapshots_and_events_changes_nothing(void);
void test_offline_runs_repeat_exactly(void);
void test_offline_refuses_what_only_a_device_host_does(void);
void test_offline_swap_fades_and_repeats_exactly(void);
void test_trace_parses_a_build_and_its_commands(void);
void test_trace_errors_name_the_line(void);
void test_trace_plays_what_a_host_renders_skipping_the_skip(void);
void test_trace_swap_plays_through_a_fade(void);

/* ---- the device layer (test_device.c) ---- */
void test_device_rate_choice(void);
void test_device_null_backend_lists_one_device(void);
void test_device_null_backend_plays_the_render_threads_audio_in_real_time(void);
void test_device_44100_runs_natively(void);
void test_device_change_starts_a_new_epoch_and_the_sound_goes_on(void);
void test_device_at_another_rate_rebuilds_the_engine_in_a_new_epoch(void);
void test_device_suspend_stops_the_device_and_resume_restarts_it_in_a_new_epoch(void);
void test_device_lost_is_noticed_and_the_default_device_is_opened_again(void);
void test_device_thread_never_runs_the_physics(void);

typedef struct test_entry {
    const char *name;
    void      (*run)(void);
} test_entry;

#define T(f) { #f, f }
static const test_entry TESTS[] = {
    T(test_ring_fifo_wrap_and_counts),
    T(test_ring_rejects_bad_sizes_and_flushes),
    T(test_ring_stress_in_order_no_loss),
    T(test_queue_fifo_refuses_when_full_and_wraps),
    T(test_queue_rejects_bad_sizes),
    T(test_queue_stress_no_loss_no_tear),
    T(test_triple_newest_wins_and_nothing_stale),
    T(test_triple_stress_never_torn),
    T(test_evring_orders_and_counts_losses),
    T(test_evring_stress_taken_plus_lost_is_everything),
    T(test_host_render_thread_fills_the_ring_to_its_target),
    T(test_host_render_thread_priority_is_set_and_reported),
    T(test_host_command_is_applied_and_acknowledged),
    T(test_host_acks_come_in_order_with_their_ids),
    T(test_host_full_command_queue_refuses_and_never_waits),
    T(test_host_refused_commands_say_why),
    T(test_host_every_command_kind_reaches_the_engine),
    T(test_host_no_command_is_lost_under_load),
    T(test_host_snapshots_are_whole_and_published_about_60_times_a_second),
    T(test_host_a_slow_reader_gets_the_newest_and_drops_are_counted),
    T(test_host_events_arrive_in_order_and_losses_are_the_gaps),
    T(test_host_clock_maps_physics_ticks_to_audio_frames),
    T(test_host_44100_renders_natively),
    T(test_host_output_time_needs_a_device_callback_in_the_epoch),
    T(test_host_underrun_fades_out_in_2ms_then_silence_then_fades_in),
    T(test_host_short_data_before_an_underrun_still_fades_to_zero),
    T(test_audit_counts_forbidden_calls_only_on_the_render_thread),
    T(test_render_thread_never_allocates_locks_or_logs),
#ifdef RB_HOST_MALLOC_WRAP
    T(test_render_thread_never_calls_the_c_libraries_allocator),
#endif
    T(test_host_deadline_bins_are_tenths_then_missed),
    T(test_host_statistics_deadline_histogram_and_ring_fill),
    T(test_swap_installs_the_new_engine_and_frees_the_old_one),
    T(test_swap_fades_without_a_click),
    T(test_swap_failed_preparation_leaves_the_running_engine_untouched),
    T(test_swap_prepare_table_is_bounded_and_unknown_ids_are_refused),
    T(test_swap_stale_generation_is_refused_and_its_engine_freed),
    T(test_swap_rapid_successive_swaps_each_get_their_own_fade_and_ack),
    T(test_swap_busy_when_engines_are_not_being_freed_then_retried),
    T(test_swap_commands_behind_it_wait_and_apply_to_the_new_engine),
    T(test_swap_concurrent_with_the_device_thread_and_readers),
    T(test_offline_mono_drive_is_the_cores_own_bit_for_bit),
    T(test_offline_stereo_scene_drive_is_the_cores_own_bit_for_bit),
    T(test_offline_publishing_snapshots_and_events_changes_nothing),
    T(test_offline_runs_repeat_exactly),
    T(test_offline_refuses_what_only_a_device_host_does),
    T(test_offline_swap_fades_and_repeats_exactly),
    T(test_trace_parses_a_build_and_its_commands),
    T(test_trace_errors_name_the_line),
    T(test_trace_plays_what_a_host_renders_skipping_the_skip),
    T(test_trace_swap_plays_through_a_fade),
    T(test_device_rate_choice),
    T(test_device_null_backend_lists_one_device),
    T(test_device_null_backend_plays_the_render_threads_audio_in_real_time),
    T(test_device_44100_runs_natively),
    T(test_device_change_starts_a_new_epoch_and_the_sound_goes_on),
    T(test_device_at_another_rate_rebuilds_the_engine_in_a_new_epoch),
    T(test_device_suspend_stops_the_device_and_resume_restarts_it_in_a_new_epoch),
    T(test_device_lost_is_noticed_and_the_default_device_is_opened_again),
    T(test_device_thread_never_runs_the_physics),
};
#undef T

static long checks, failures;
static int current_failed;

void ht_check(int ok, const char *expr, const char *file, int line)
{
    checks++;
    if (!ok) {
        failures++;
        current_failed = 1;
        printf("    %s:%d: CHECK(%s) failed\n", file, line, expr);
    }
}

void ht_check_int(long long a, long long b, const char *ea, const char *eb, const char *file, int line)
{
    checks++;
    if (a != b) {
        failures++;
        current_failed = 1;
        printf("    %s:%d: %s == %s failed: %lld vs %lld\n", file, line, ea, eb, a, b);
    }
}

void ht_check_near(double a, double b, double tol, const char *ea, const char *eb, const char *file, int line)
{
    checks++;
    if (!(fabs(a - b) <= tol)) {
        failures++;
        current_failed = 1;
        printf("    %s:%d: |%s - %s| <= %g failed: %.9g vs %.9g\n", file, line, ea, eb, tol, a, b);
    }
}

static int wanted(const char *name, int argc, char **argv)
{
    if (argc <= 1) return 1;
    for (int i = 1; i < argc; i++)
        if (strstr(name, argv[i])) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    const int count = (int)(sizeof TESTS / sizeof TESTS[0]);
    if (argc == 2 && !strcmp(argv[1], "--list")) {
        for (int i = 0; i < count; i++) puts(TESTS[i].name);
        return 0;
    }
    int ran = 0, failed_tests = 0;
    for (int i = 0; i < count; i++) {
        if (!wanted(TESTS[i].name, argc, argv)) continue;
        current_failed = 0;
        TESTS[i].run();
        fflush(stdout);
        printf("%s  %s\n", current_failed ? "FAIL " : "ok   ", TESTS[i].name);
        fflush(stdout);
        ran++;
        failed_tests += current_failed;
    }
    printf("\n%d tests, %ld checks, %d failed\n", ran, checks, failed_tests);
    if (ran == 0) {
        printf("no test matches\n");
        return 2;
    }
    return failed_tests ? 1 : 0;
}
