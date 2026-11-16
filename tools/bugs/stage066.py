"""Stage 066: the host's capture tap can fail its tests.

python tools/plant_bugs.py 066 WORKDIR all      (a few at a time, under `timeout`)
The tap is the stage's evidence (what the device was given, compared with an offline render of
the drive's trace), so each bug is a way it could lie: a block missing, a block written twice,
a gap not counted, frames numbered wrong, a tap that is there when nobody asked. The app's side
(the dead-man rule, the throttle styles, the trace recorder) is TypeScript, tested by vitest:
its bugs are in docs/stage-066.md. Caught by rb_host_tests (TARGET); a test name here is a
substring of the names the program runs.
"""
TARGET = "rb_host_tests"

TAP = ["test_host_capture_tap"]

BUGS = {
    'capture-never-written': ("host/src/runner.c",
        "            rbh_capture_block(h, block, RB_HOST_BLOCK_FRAMES);\n",
        "",
        TAP, 300),
    'capture-written-twice': ("host/src/runner.c",
        "            rbh_capture_block(h, block, RB_HOST_BLOCK_FRAMES);\n",
        "            rbh_capture_block(h, block, RB_HOST_BLOCK_FRAMES);\n"
        "            rbh_capture_block(h, block, RB_HOST_BLOCK_FRAMES);\n",
        TAP, 300),
    'capture-drops-not-counted': ("host/src/runner.c",
        "        add(&h->stats.capture_dropped, 1);\n",
        "",
        TAP, 300),
    'capture-first-frame-wrong': ("host/src/host.c",
        "    if (first_frame) *first_frame = rbh_ring_consumed(&h->capture);",
        "    if (first_frame) *first_frame = rbh_ring_written(&h->capture);",
        TAP, 300),
    'capture-any-size': ("host/src/host.c",
        "        && (cfg.capture_frames < 1024 || cfg.capture_frames > (1 << 24) || !is_pow2((uint32_t)cfg.capture_frames)))",
        "        && (cfg.capture_frames < 0))",
        TAP, 300),
    'capture-reads-without-a-tap': ("host/src/host.c",
        "    if (!h->capture.capacity) return RB_HOST_ERR_STATE;\n",
        "    if (!h->capture.capacity) return 0;\n",
        TAP, 300),
}
