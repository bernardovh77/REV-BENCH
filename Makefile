# REV-BENCH build
#
#   make              build the command-line tool (build/rb_cli)
#   make test         build and run the unit tests
#   make render       render all standard examples plus five long coasts; finish
#                     with the single offline renders/RESULTS.html. Replaces
#                     only renders/current/ after the complete run succeeds.
#   make render-standard   standard examples only (for isolated regression sets):
#                     the four starting and
#                     idling, pulling into its rev limiter and back to idle,
#                     and revved in neutral, heard from behind the car, idling
#                     over its open bonnet, and from the driver's seat idling
#                     and pulling; the four at 3000 rpm on a dyno with each
#                     muffler, 1 m from its tailpipe; both V8s at 3000 rpm, the
#                     cross-plane one from every place around the car and in
#                     it, pulling and revved (the mufflers' and the V8s' at
#                     reduced gain to keep headroom below the soft limiter);
#                     the six and the boxer revved; the boxer with
#                     its unequal headers and with equal ones; and the test
#                     tone. Every cycle its own (stage 12c): rb_cli sound's
#                     --variation 0 removes randomness from the same charge model
#   make wasm         build the core for the browser (web/src/wasm/rb_core.wasm)
#   make fuzz         random engines (a preset with random fields), 20 of them
#                     for 10 s each on their own crank: nothing may stop being a
#                     number or run away (several minutes)
#   make bench        how fast the core runs: each preset held wide open at
#                     3000 rpm, then the pieces the profile points at
#   make bench-wasm   the same as WebAssembly, run by Node (needs zig and node)
#   make bench-render the app's whole render path (rb_render_stereo through rb.h)
#                     for each preset, quality and drive (stage 118); the
#                     interleaved table: tools/bench_render.py. bench-render-wasm
#                     is the same as WebAssembly (BENCH_RENDER_ARGS selects cells)
#   make test-wasm    run gas/batched-cell/pipe checks in the SIMD WASM build
#   make audit        five-preset S1 physics baseline to renders/s1-audit/physics.csv
#   make diagnostics-check   five-preset guard/flux/trajectory/cost proof
#   make clean        delete build output
#   make host-test    build and run the real-time audio host's tests (stage 123,
#                     on miniaudio's null device)
#   make host-tools   rb_host_render, rb_host_stress and rb_host_devices
#   make help         show this text
#
# Options:
#   make SAN=1 test   build with AddressSanitizer + UBSan (catches memory
#                     bugs and undefined behaviour), output in build-san/
#   make TSAN=1 host-test   the host's tests under ThreadSanitizer (gcc or clang
#                     on Linux or macOS, WSL included), output in build-tsan/
#   make BUILD=dir    put build output in another folder
#   make render-standard RENDER_DIR=dir RENDER_FLAGS=--float   isolated float set
#   make results      rebuild RESULTS.html from current recordings without rerendering
#   make render PYTHON=path/to/python   choose Python (default: ~/rb-venv's, if there)
#   make wasm ZIG=... use a specific zig (default: `zig` if it is on PATH,
#                     else `$(PYTHON) -m ziglang` from: pip install -r tools/requirements-dev.txt)

ifeq ($(SAN),1)
  # the sanitizers need their runtime libraries, which gcc and clang have on Linux
  # and macOS (WSL too) but w64devkit doesn't: say so instead of failing to link
  SAN_LINKS := $(shell f=$$(mktemp 2>/dev/null || echo san-probe); \
      echo 'int main(void){return 0;}' | $(CC) -fsanitize=address,undefined -x c - -o "$$f" \
      >/dev/null 2>&1 && echo yes; rm -f "$$f" "$$f.exe")
  ifneq ($(SAN_LINKS),yes)
    $(error SAN=1 needs a compiler with the sanitizer runtimes (gcc or clang on Linux or macOS, WSL included); $(CC) here can't link them (w64devkit has none))
  endif
  BUILD   ?= build-san
  CFLAGS  ?= -O1 -g
  CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS += -fsanitize=address,undefined
endif

ifeq ($(TSAN),1)
  # ThreadSanitizer, for the host's concurrent tests (stage 123): like SAN, it needs the
  # runtime library that gcc and clang have on Linux and macOS and w64devkit doesn't
  TSAN_LINKS := $(shell echo 'int main(void){return 0;}' | $(CC) -fsanitize=thread -x c - -o /dev/null \
      >/dev/null 2>&1 && echo yes)
  ifneq ($(TSAN_LINKS),yes)
    $(error TSAN=1 needs a compiler with the ThreadSanitizer runtime (gcc or clang on Linux or macOS, WSL included); $(CC) here can't link it)
  endif
  BUILD   ?= build-tsan
  CFLAGS  ?= -O1 -g
  CFLAGS  += -fsanitize=thread -fno-omit-frame-pointer
  LDFLAGS += -fsanitize=thread
endif

BUILD    ?= build
# Optional isolated standard sets; full checkpoints use the publisher below.
RENDER_DIR   ?= renders/work/standard
RENDER_FLAGS ?= --float
# The venv of tools/requirements-dev.txt (docs/workspace.md) when there is one, so its
# zig and analysis packages are used: ~/rb-venv/bin on Linux, Scripts on Windows.
# Else python3 where it runs, else python (Windows' python3 can be the Store's stub).
RB_VENV      ?= $(subst \,/,$(firstword $(HOME) $(USERPROFILE)))/rb-venv
VENV_PYTHON  := $(firstword $(wildcard $(RB_VENV)/bin/python $(RB_VENV)/Scripts/python.exe))
PYTHON       ?= $(if $(VENV_PYTHON),$(VENV_PYTHON),$(shell python3 -c "" >/dev/null 2>&1 && echo python3 || echo python))
CFLAGS   ?= -O3 -g
CFLAGS   += -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
# Strict double arithmetic: keep NaN checks and each cell's operation order.
# Disable LTO with LTO=0 for tools/profilers that require separate objects.
# It is on by default where the compiler has it: w64devkit's gcc on Windows
# doesn't (stage 119), and compiling with -flto there fails.
LTO ?= $(if $(shell echo 'int x;' | $(CC) -flto -x c -S -o - - >/dev/null 2>&1 && echo yes),1,0)
ifneq ($(filter 1,$(SAN) $(TSAN)),1)
ifeq ($(LTO),1)
  CFLAGS  += -flto
  LDFLAGS += -flto
endif
endif
CFLAGS += -ffp-contract=off
CPPFLAGS += -Icore/include -Icore/src -Inative -Iexperiments/acoustics -MMD -MP
LDLIBS   += -lm

CORE_SRC   := $(wildcard core/src/*.c)
CORE_HDR   := $(wildcard core/include/*.h core/src/*.h)
NATIVE_SRC := native/wav.c native/csv.c native/fuzz.c
CLI_SRC    := native/rb_cli.c native/sound.c
TEST_SRC   := $(wildcard tests/*.c)

obj = $(patsubst %.c,$(BUILD)/%.o,$(1))
CORE_OBJ   := $(call obj,$(CORE_SRC))
NATIVE_OBJ := $(call obj,$(NATIVE_SRC))
CLI_OBJ    := $(call obj,$(CLI_SRC))
TEST_OBJ   := $(call obj,$(TEST_SRC))
BENCH_OBJ  := $(call obj,native/rb_bench.c)
BENCH_RENDER_OBJ := $(call obj,native/bench_render.c)
AUDIT_OBJ  := $(call obj,native/rb_audit.c)
ACOUSTIC_SRC := experiments/acoustics/wave.c experiments/acoustics/euler.c experiments/acoustics/coupled_duct.c experiments/acoustics/mixture.c experiments/acoustics/gas_graph.c experiments/acoustics/thermal.c experiments/acoustics/material.c experiments/acoustics/fixed_transport.c experiments/acoustics/junction.c
ACOUSTIC_OBJ := $(call obj,$(ACOUSTIC_SRC))
ACOUSTIC_HDR := $(wildcard experiments/acoustics/*.h)
ACOUSTIC_PROBE_OBJ := $(call obj,tools/acoustic_candidate_probe.c)
COUPLED_PROBE_OBJ := $(call obj,tools/coupled_duct_probe.c)
BUILD_CONTRACT_OBJ := $(call obj,tools/build_contract_probe.c)
DIAGNOSTICS_OBJ := $(call obj,tools/numerical_diagnostics_probe.c)
IGNITION_OBJ := $(call obj,tools/ignition_causality_probe.c)
IGNITION_SCENARIO_OBJ := $(call obj,tools/ignition_scenarios_probe.c)
COMBUSTION_VARIATION_OBJ := $(call obj,tools/combustion_variation_probe.c)
INJECTION_SCENARIO_OBJ := $(call obj,tools/injection_scenarios_probe.c)
MIXTURE_PROBE_OBJ := $(call obj,tools/mixture_probe.c)
MIXTURE_ENGINE_OBJ := $(call obj,tools/mixture_engine_control.c)
EXHAUST_GEOMETRY_OBJ := $(call obj,tools/exhaust_geometry_probe.c)
MATERIAL_PROBE_OBJ := $(call obj,tools/material_probe.c)
FIXED_TRANSPORT_OBJ := $(call obj,tools/fixed_transport_probe.c)
JUNCTION_PROBE_OBJ := $(call obj,tools/junction_probe.c)
THERMAL_BENCH_OBJ := $(call obj,tools/thermal_bench.c)
THERMAL_PROBE_OBJ := $(call obj,tools/thermal_probe.c)
GAS_GRAPH_PROBE_OBJ := $(call obj,tools/gas_graph_probe.c)
# Every object, for the header dependencies (-MMD) included at the end: one
# left out isn't rebuilt when a header changes (stage 13: rb_bench.o, a segfault).
ALL_OBJ    := $(CORE_OBJ) $(NATIVE_OBJ) $(CLI_OBJ) $(TEST_OBJ) $(BENCH_OBJ) $(BENCH_RENDER_OBJ) $(AUDIT_OBJ) $(ACOUSTIC_OBJ) $(ACOUSTIC_PROBE_OBJ) $(COUPLED_PROBE_OBJ) $(BUILD_CONTRACT_OBJ) $(DIAGNOSTICS_OBJ) $(IGNITION_OBJ) $(IGNITION_SCENARIO_OBJ) $(COMBUSTION_VARIATION_OBJ) $(INJECTION_SCENARIO_OBJ) $(MIXTURE_PROBE_OBJ) $(MIXTURE_ENGINE_OBJ) $(EXHAUST_GEOMETRY_OBJ) $(GAS_GRAPH_PROBE_OBJ) $(THERMAL_PROBE_OBJ) $(THERMAL_BENCH_OBJ) $(MATERIAL_PROBE_OBJ) $(FIXED_TRANSPORT_OBJ) $(JUNCTION_PROBE_OBJ)

CLI   := $(BUILD)/rb_cli
TESTS := $(BUILD)/rb_tests
BENCH := $(BUILD)/rb_bench
BENCH_RENDER := $(BUILD)/rb_bench_render
AUDIT := $(BUILD)/rb_audit
ACOUSTIC_PROBE := $(BUILD)/acoustic_candidate_probe

.PHONY: all test test-wasm render render-standard results wasm bench bench-render bench-wasm bench-render-wasm audit fuzz clean help acoustic-probe

all: $(CLI)

$(CLI): $(CLI_OBJ) $(CORE_OBJ) $(NATIVE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TESTS): $(TEST_OBJ) $(CORE_OBJ) $(NATIVE_OBJ) $(ACOUSTIC_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(ACOUSTIC_PROBE): $(ACOUSTIC_PROBE_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ) $(call obj,native/wav.c)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

acoustic-probe: $(ACOUSTIC_PROBE)

$(BUILD)/build_contract_probe: $(BUILD_CONTRACT_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/numerical_diagnostics_probe: $(DIAGNOSTICS_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/ignition_causality_probe: $(IGNITION_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/ignition_scenarios_probe: $(IGNITION_SCENARIO_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/combustion_variation_probe: $(COMBUSTION_VARIATION_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/injection_scenarios_probe: $(INJECTION_SCENARIO_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/mixture_probe: $(MIXTURE_PROBE_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/mixture_engine_control: $(MIXTURE_ENGINE_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/exhaust_geometry_probe: $(EXHAUST_GEOMETRY_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/gas_graph_probe: $(GAS_GRAPH_PROBE_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: diagnostics-check
diagnostics-check: $(BUILD)/numerical_diagnostics_probe
	$(PYTHON) tools/check_numerical_diagnostics.py --probe $< \
	  --output renders/evidence/stage-038/measurements.json

.PHONY: build-contract-check
build-contract-check: $(BUILD)/build_contract_probe
	$(PYTHON) tools/check_build_contract.py --probe $(BUILD)/build_contract_probe

$(BUILD)/coupled_duct_probe: $(COUPLED_PROBE_OBJ) $(call obj,experiments/acoustics/coupled_duct.c)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: coupled-probe
coupled-probe: $(BUILD)/coupled_duct_probe

$(BENCH): $(BENCH_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(AUDIT): $(AUDIT_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

comma := ,
# ---- the real-time audio host (stage 123) ----
# host/ sees only the public headers (core/include), as rb_cli sound does, and
# miniaudio is compiled apart, without the core's warnings. The system layer is
# host/src/platform_win.c or platform_posix.c.
ifeq ($(OS),Windows_NT)
  HOST_PLAT   := host/src/platform_win.c
  HOST_LIBS   := -lavrt -lwinmm -lpsapi
else
  HOST_PLAT   := host/src/platform_posix.c
  HOST_LIBS   := -lpthread -ldl
endif
HOST_MA_SRC   := host/src/miniaudio_impl.c
HOST_SRC      := $(filter-out host/src/platform_%.c $(HOST_MA_SRC),$(wildcard host/src/*.c)) $(HOST_PLAT)
HOST_TEST_SRC := $(wildcard host/tests/*.c)
HOST_TOOL_SRC := $(wildcard host/tools/*.c)
HOST_OBJ      := $(call obj,$(HOST_SRC))
HOST_MA_OBJ   := $(call obj,$(HOST_MA_SRC))
HOST_TEST_OBJ := $(call obj,$(HOST_TEST_SRC))
HOST_TOOL_OBJ := $(call obj,$(HOST_TOOL_SRC))
ALL_OBJ       += $(HOST_OBJ) $(HOST_MA_OBJ) $(HOST_TEST_OBJ) $(HOST_TOOL_OBJ)
HOST_TOOLS    := $(patsubst host/tools/%.c,$(BUILD)/rb_%,$(HOST_TOOL_SRC))
HOST_TESTS    := $(BUILD)/rb_host_tests
HOST_FILTER   ?=
# the test binary also counts the C library's allocator calls the render thread makes (GNU ld's
# --wrap; not macOS, whose linker has none)
HOST_WRAP     ?= $(if $(filter Darwin,$(shell uname -s)),,1)
HOST_WRAP_LD  := $(if $(HOST_WRAP),-Wl$(comma)--wrap=malloc -Wl$(comma)--wrap=calloc -Wl$(comma)--wrap=realloc -Wl$(comma)--wrap=free)
$(HOST_OBJ) $(HOST_TOOL_OBJ): CPPFLAGS := -Icore/include -Ihost/include -Ihost/src -isystem third_party/miniaudio -Inative -MMD -MP
$(HOST_TEST_OBJ): CPPFLAGS := -Icore/include -Ihost/include -Ihost/src -Inative -MMD -MP $(if $(HOST_WRAP),-DRB_HOST_MALLOC_WRAP)
$(HOST_MA_OBJ): CPPFLAGS := -Ithird_party/miniaudio -MMD -MP
$(HOST_MA_OBJ): CFLAGS := -O2 -g0 -std=gnu11 -w -ffp-contract=off

$(HOST_TESTS): $(HOST_TEST_OBJ) $(HOST_OBJ) $(HOST_MA_OBJ) $(CORE_OBJ) $(call obj,native/wav.c)
	$(CC) $(LDFLAGS) $(HOST_WRAP_LD) $^ $(LDLIBS) $(HOST_LIBS) -o $@

# a tool is its own object plus the host, the core and the WAV writer
$(HOST_TOOLS): $(BUILD)/rb_%: $(BUILD)/host/tools/%.o $(HOST_OBJ) $(HOST_MA_OBJ) $(CORE_OBJ) $(call obj,native/wav.c)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(HOST_LIBS) -o $@

.PHONY: host-test host-tools
host-test: $(HOST_TESTS)
	RBT_TMPDIR=$(BUILD) $(HOST_TESTS) $(HOST_FILTER)

host-tools: $(HOST_TOOLS)

# rb_cli sound drives the engine through the public API alone (stage 121), as
# the desktop app will: it compiles without core/src on the include path, so
# an internal header there is a build error, not a habit.
$(BUILD)/native/sound.o: CPPFLAGS := -Icore/include -Inative -MMD -MP

$(BUILD)/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TESTS)
	RBT_TMPDIR=$(BUILD) $(TESTS)

bench: $(BENCH)
	$(BENCH)

# Stage 118: the app's whole render path (rb_render_stereo through rb.h), each
# preset, quality and drive. tools/bench_render.py runs it interleaved.
bench-render: $(BENCH_RENDER)
	$(BENCH_RENDER)

$(BENCH_RENDER): $(BENCH_RENDER_OBJ) $(CORE_OBJ) $(call obj,native/wav.c)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

audit: $(AUDIT)
	@mkdir -p renders/s1-audit
	$(AUDIT) > renders/s1-audit/physics.csv.tmp
	mv renders/s1-audit/physics.csv.tmp renders/s1-audit/physics.csv

fuzz: $(CLI)
	$(CLI) fuzz --sets 20 --seconds 10

render: $(CLI)
	$(PYTHON) tools/render_results.py --cli "$(CLI)" --build "$(BUILD)"

results:
	$(PYTHON) tools/render_results.py --publish-only

render-standard: $(CLI)
	@mkdir -p "$(RENDER_DIR)"
	$(CLI) tone $(RENDER_FLAGS) --out $(RENDER_DIR)/test.wav --csv $(RENDER_DIR)/test.csv
	$(CLI) sound $(RENDER_FLAGS) --preset i4 --idle --out $(RENDER_DIR)/i4-idle.wav
	$(CLI) sound $(RENDER_FLAGS) --preset i4 --sweep --out $(RENDER_DIR)/i4-sweep.wav --csv $(RENDER_DIR)/i4-sweep.csv
	$(CLI) sound $(RENDER_FLAGS) --preset i4 --rev --out $(RENDER_DIR)/i4-rev.wav --csv $(RENDER_DIR)/i4-rev.csv
	$(CLI) sound $(RENDER_FLAGS) --preset i4 --idle --listener front --out $(RENDER_DIR)/i4-idle-front.wav
	$(CLI) sound $(RENDER_FLAGS) --preset i4 --idle --listener cabin --out $(RENDER_DIR)/i4-idle-cabin.wav
	$(CLI) sound $(RENDER_FLAGS) --preset i4 --sweep --listener cabin --gain 0.5 --out $(RENDER_DIR)/i4-sweep-cabin.wav
	for m in none straight chambered; do \
	  $(CLI) sound $(RENDER_FLAGS) --preset i4 --hold --rpm 3000 --muffler $$m --listener tailpipe --gain 0.5 \
	    --out $(RENDER_DIR)/i4-3000-$$m.wav || exit 1; done
	for l in behind tailpipe side front cabin; do \
	  $(CLI) sound $(RENDER_FLAGS) --preset v8-cross --hold --rpm 3000 --listener $$l --gain 0.2 \
	    --out $(RENDER_DIR)/v8-cross-3000-$$l.wav || exit 1; done
	$(CLI) sound $(RENDER_FLAGS) --preset v8-cross --sweep --gain 0.2 --out $(RENDER_DIR)/v8-cross-sweep.wav \
	  --csv $(RENDER_DIR)/v8-cross-sweep.csv
	$(CLI) sound $(RENDER_FLAGS) --preset v8-cross --rev --gain 0.2 --out $(RENDER_DIR)/v8-cross-rev.wav
	$(CLI) sound $(RENDER_FLAGS) --preset v8-flat --hold --rpm 3000 --gain 0.2 --out $(RENDER_DIR)/v8-flat-3000.wav
	$(CLI) sound $(RENDER_FLAGS) --preset v8-flat --rev --gain 0.25 --out $(RENDER_DIR)/v8-flat-rev.wav
	$(CLI) sound $(RENDER_FLAGS) --preset i6 --rev --gain 0.2 --out $(RENDER_DIR)/i6-rev.wav
	$(CLI) sound $(RENDER_FLAGS) --preset boxer4 --rev --out $(RENDER_DIR)/boxer4-rev.wav
	$(CLI) sound $(RENDER_FLAGS) --preset boxer4 --hold --rpm 3000 --out $(RENDER_DIR)/boxer4-3000.wav
	$(CLI) sound $(RENDER_FLAGS) --preset boxer4 --hold --rpm 3000 --equal-headers --out $(RENDER_DIR)/boxer4-3000-equal.wav

# ---- WebAssembly build for the browser ----------------------------------
# A standalone module: libc/libm are compiled in, it needs no imports and has
# no JavaScript glue. The output is committed, so the web app works on a
# machine without zig.
ZIG      ?= $(shell command -v zig >/dev/null 2>&1 && echo zig || echo $(PYTHON) -m ziglang)
WASM_OUT ?= web/src/wasm/rb_core.wasm
# Fingerprint of the C sources the .wasm was built from. The web unit tests
# recompute it, so they fail if the C code changed without a `make wasm`.
WASM_SUM ?= $(WASM_OUT:.wasm=.sources.sha256)
# Every public function of core/include/rb.h. New API functions go here too.
WASM_EXPORTS := rb_version rb_config_default rb_sizeof rb_init rb_reset \
    rb_create rb_create_with rb_config_quality rb_create_with_quality rb_destroy \
    rb_step rb_render_block rb_render_stereo rb_set_scene \
    rb_set_param rb_get_param rb_param_count rb_param_name rb_param_unit \
    rb_param_min rb_param_max rb_param_default rb_param_flags rb_param_find \
    rb_get_telemetry rb_telemetry_count rb_telemetry_name rb_telemetry_unit \
    rb_set_quality rb_get_quality rb_variation_seed rb_listener_source_distance rb_set_scene_time \
    rb_dyno_attach rb_dyno_release rb_dyno_attached rb_dyno_set_sweep rb_dyno_clear rb_get_dyno \
    rb_snapshot_size rb_get_snapshot rb_event_kind_name rb_read_events rb_events_lost \
    rb_preset_count rb_preset_key rb_preset_name rb_preset_find rb_preset_quality rb_load_preset rb_get_preset \
    rb_get_engine rb_set_engine rb_engine_field_size rb_engine_field_count \
    rb_engine_field_name rb_engine_field_unit rb_engine_field_flags \
    rb_engine_field_min rb_engine_field_max rb_engine_field_find \
    rb_get_cylinder rb_cylinder_field_count rb_cylinder_field_name rb_cylinder_field_unit \
    rb_pipe_count rb_get_pipe rb_pipe_field_count rb_pipe_field_name rb_pipe_field_unit \
    rb_get_pipe_cell rb_pipe_cell_field_count rb_pipe_cell_field_name rb_pipe_cell_field_unit \
    rb_sample_rate rb_oversample rb_physics_dt rb_pipe_rate rb_alloc_floats rb_free_floats
comma := ,
WASM_OPT ?= -O3 -msimd128 -flto -ffp-contract=off
WASM_FLAGS := -target wasm32-wasi -mexec-model=reactor $(WASM_OPT) -g0 -std=c11 \
    -Wall -Wextra -Icore/include -Icore/src -Wl$(comma)--strip-debug \
    $(foreach f,$(WASM_EXPORTS),-Wl$(comma)--export=$(f))

wasm: $(BUILD)/rb_core.wasm
	@mkdir -p $(dir $(WASM_OUT))
	cp $< $(WASM_OUT)
	@cat $(sort $(CORE_SRC) $(CORE_HDR)) | sha256sum | cut -d' ' -f1 > $(WASM_SUM)
	@echo "wrote $(WASM_OUT) ($$(wc -c < $(WASM_OUT)) bytes) and $(WASM_SUM)"

$(BUILD)/rb_core.wasm: $(CORE_SRC) $(CORE_HDR) Makefile
	@mkdir -p $(@D)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc $(WASM_FLAGS) $(CORE_SRC) -o $@

# The benchmark as a WASI program (it has a main and reads the clock), built
# like the core: Node runs it (tools/bench_wasm.mjs).
bench-wasm: $(BUILD)/rb_bench.wasm
	node tools/bench_wasm.mjs $<

$(BUILD)/rb_bench.wasm: native/rb_bench.c $(CORE_SRC) $(CORE_HDR) Makefile
	@mkdir -p $(@D)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -Wall -Wextra -Icore/include -Icore/src native/rb_bench.c $(CORE_SRC) -o $@

# Stage 118: the app's render path (native/bench_render.c) as WebAssembly, for
# the record: the same drives and the same hashes as the native program's.
bench-render-wasm: $(BUILD)/rb_bench_render.wasm
	node tools/bench_wasm.mjs $< $(BENCH_RENDER_ARGS)

$(BUILD)/rb_bench_render.wasm: native/bench_render.c native/wav.c $(CORE_SRC) $(CORE_HDR) Makefile
	@mkdir -p $(@D)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -Wall -Wextra -Icore/include -Icore/src -Inative native/bench_render.c native/wav.c $(CORE_SRC) -o $@

# These tests need no filesystem access: the same C assertions exercise the
# actual WebAssembly SIMD and scalar-fallback code, not only its exports.
WASM_TEST_FILTER ?= chamber_batch gas pipe_acoustic_transport pipe_conserves_everything pipe_two_rates_agree
test-wasm: $(BUILD)/rb_tests.wasm
	node tools/bench_wasm.mjs $< $(WASM_TEST_FILTER)

$(BUILD)/rb_tests.wasm: $(TEST_SRC) $(CORE_SRC) $(CORE_HDR) $(NATIVE_SRC) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) tests/rb_test.h Makefile
	@mkdir -p $(@D)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -Wall -Wextra -Icore/include -Icore/src -Inative -Iexperiments/acoustics \
	  $(TEST_SRC) $(CORE_SRC) $(NATIVE_SRC) $(ACOUSTIC_SRC) -o $@

$(BUILD)/exhaust_geometry_probe.wasm: tools/exhaust_geometry_probe.c $(CORE_SRC) $(CORE_HDR) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) Makefile
	@mkdir -p $(@D)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -Wall -Wextra -Icore/include -Icore/src -Iexperiments/acoustics \
	  tools/exhaust_geometry_probe.c $(CORE_SRC) $(ACOUSTIC_SRC) -o $@

clean:
	rm -rf build build-san

help:
	@awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' Makefile

-include $(ALL_OBJ:.o=.d)

$(BUILD)/gas_graph_probe.wasm: tools/gas_graph_probe.c $(CORE_SRC) $(CORE_HDR) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) Makefile
	@mkdir -p $(BUILD)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -ffp-contract=off -Icore/include -Icore/src -Inative -Iexperiments/acoustics \
	  tools/gas_graph_probe.c $(CORE_SRC) $(ACOUSTIC_SRC) -o $@

$(BUILD)/thermal_probe: $(THERMAL_PROBE_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/thermal_probe.wasm: tools/thermal_probe.c $(CORE_SRC) $(CORE_HDR) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) Makefile
	@mkdir -p $(BUILD)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -ffp-contract=off -Icore/include -Icore/src -Inative -Iexperiments/acoustics \
	  tools/thermal_probe.c $(CORE_SRC) $(ACOUSTIC_SRC) -o $@

$(BUILD)/thermal_bench: $(THERMAL_BENCH_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/material_probe: $(MATERIAL_PROBE_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/fixed_transport_probe: $(FIXED_TRANSPORT_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/fixed_transport_probe.wasm: tools/fixed_transport_probe.c $(CORE_SRC) $(CORE_HDR) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) Makefile
	@mkdir -p $(BUILD)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -ffp-contract=off -Icore/include -Icore/src -Inative -Iexperiments/acoustics \
	  tools/fixed_transport_probe.c $(CORE_SRC) $(ACOUSTIC_SRC) -o $@

$(BUILD)/material_probe.wasm: tools/material_probe.c $(CORE_SRC) $(CORE_HDR) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) Makefile
	@mkdir -p $(BUILD)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -ffp-contract=off -Icore/include -Icore/src -Inative -Iexperiments/acoustics \
	  tools/material_probe.c $(CORE_SRC) $(ACOUSTIC_SRC) -o $@

$(BUILD)/junction_probe: $(JUNCTION_PROBE_OBJ) $(ACOUSTIC_OBJ) $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/junction_probe.wasm: tools/junction_probe.c $(CORE_SRC) $(CORE_HDR) $(ACOUSTIC_SRC) $(ACOUSTIC_HDR) Makefile
	@mkdir -p $(BUILD)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc -target wasm32-wasi $(WASM_OPT) -g0 -std=c11 \
	  -ffp-contract=off -Icore/include -Icore/src -Inative -Iexperiments/acoustics \
	  tools/junction_probe.c $(CORE_SRC) $(ACOUSTIC_SRC) -o $@
