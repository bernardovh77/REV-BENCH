# REV-BENCH build
#
#   make              build the command-line tool (build/rb_cli)
#   make test         build and run the unit tests
#   make render       render the current test sound to renders/test.wav
#   make wasm         build the core for the browser (web/src/wasm/rb_core.wasm)
#   make clean        delete build output
#   make help         show this text
#
# Options:
#   make SAN=1 test   build with AddressSanitizer + UBSan (catches memory
#                     bugs and undefined behaviour), output in build-san/
#   make BUILD=dir    put build output in another folder
#   make wasm ZIG=... use a specific zig (default: `zig` if it is on PATH,
#                     else `python3 -m ziglang` from: pip install ziglang)

ifeq ($(SAN),1)
  BUILD   ?= build-san
  CFLAGS  ?= -O1 -g
  CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS += -fsanitize=address,undefined
endif

BUILD    ?= build
CFLAGS   ?= -O2 -g
CFLAGS   += -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
CPPFLAGS += -Icore/include -Icore/src -Inative -MMD -MP
LDLIBS   += -lm

CORE_SRC   := $(wildcard core/src/*.c)
CORE_HDR   := $(wildcard core/include/*.h core/src/*.h)
NATIVE_SRC := native/wav.c native/csv.c
CLI_SRC    := native/rb_cli.c
TEST_SRC   := $(wildcard tests/*.c)

obj = $(patsubst %.c,$(BUILD)/%.o,$(1))
CORE_OBJ   := $(call obj,$(CORE_SRC))
NATIVE_OBJ := $(call obj,$(NATIVE_SRC))
CLI_OBJ    := $(call obj,$(CLI_SRC))
TEST_OBJ   := $(call obj,$(TEST_SRC))
ALL_OBJ    := $(CORE_OBJ) $(NATIVE_OBJ) $(CLI_OBJ) $(TEST_OBJ)

CLI   := $(BUILD)/rb_cli
TESTS := $(BUILD)/rb_tests

.PHONY: all test render wasm clean help

all: $(CLI)

$(CLI): $(CLI_OBJ) $(CORE_OBJ) $(NATIVE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TESTS): $(TEST_OBJ) $(CORE_OBJ) $(NATIVE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TESTS)
	RBT_TMPDIR=$(BUILD) $(TESTS)

render: $(CLI)
	@mkdir -p renders
	$(CLI) tone --out renders/test.wav --csv renders/test.csv

# ---- WebAssembly build for the browser ----------------------------------
# A standalone module: libc/libm are compiled in, it needs no imports and has
# no JavaScript glue. The output is committed, so the web app works on a
# machine without zig.
ZIG      ?= $(shell command -v zig >/dev/null 2>&1 && echo zig || echo python3 -m ziglang)
WASM_OUT ?= web/src/wasm/rb_core.wasm
# Fingerprint of the C sources the .wasm was built from. The web unit tests
# recompute it, so they fail if the C code changed without a `make wasm`.
WASM_SUM ?= $(WASM_OUT:.wasm=.sources.sha256)
# Every public function of core/include/rb.h. New API functions go here too.
WASM_EXPORTS := rb_version rb_config_default rb_sizeof rb_init rb_reset \
    rb_create rb_create_with rb_destroy rb_step rb_render_block \
    rb_set_param rb_get_param rb_param_count rb_param_name rb_param_unit \
    rb_param_min rb_param_max rb_param_default rb_param_flags rb_param_find \
    rb_get_telemetry rb_telemetry_count rb_telemetry_name \
    rb_preset_count rb_preset_key rb_preset_name rb_load_preset rb_get_preset \
    rb_get_engine rb_set_engine rb_engine_field_size rb_engine_field_count \
    rb_engine_field_name rb_engine_field_unit rb_engine_field_flags \
    rb_engine_field_min rb_engine_field_max \
    rb_get_cylinder rb_cylinder_field_count rb_cylinder_field_name rb_cylinder_field_unit \
    rb_sample_rate rb_oversample rb_physics_dt rb_alloc_floats rb_free_floats
comma := ,
WASM_FLAGS := -target wasm32-wasi -mexec-model=reactor -O3 -g0 -std=c11 \
    -Wall -Wextra -Icore/include -Icore/src -Wl$(comma)--strip-debug \
    $(foreach f,$(WASM_EXPORTS),-Wl$(comma)--export=$(f))

wasm: $(BUILD)/rb_core.wasm
	@mkdir -p $(dir $(WASM_OUT))
	cp $< $(WASM_OUT)
	@cat $(sort $(CORE_SRC) $(CORE_HDR)) | sha256sum | cut -d' ' -f1 > $(WASM_SUM)
	@echo "wrote $(WASM_OUT) ($$(wc -c < $(WASM_OUT)) bytes) and $(WASM_SUM)"

$(BUILD)/rb_core.wasm: $(CORE_SRC) $(CORE_HDR)
	@mkdir -p $(@D)
	ZIG_LOCAL_CACHE_DIR=$(BUILD)/zig-cache $(ZIG) cc $(WASM_FLAGS) $(CORE_SRC) -o $@

clean:
	rm -rf build build-san

help:
	@awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' Makefile

-include $(ALL_OBJ:.o=.d)
