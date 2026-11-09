# REV-BENCH's CMake build (stage 122): the core, the tools and the C tests
# with each system's own compiler, MSVC on Windows (the app's target), gcc on
# Linux, Apple clang on macOS. The Makefile stays for the existing workflow
# (gcc, w64devkit, the WebAssembly build); this builds the same sources with
# the same rules:
#   - strict double arithmetic everywhere: no fused multiply-add contraction,
#     no fast-math (docs/app-design.md section 8);
#   - the core's own code builds without a warning (RB_WERROR, on by default);
#   - rb_cli sound (native/sound.c) sees the public headers only (stage 121).
#
#   cmake -S . -B ~/rb-build/cmake -G Ninja
#   cmake --build ~/rb-build/cmake
#   ctest --test-dir ~/rb-build/cmake
# On Windows from any shell: python tools/msvc_env.py cmake ... (it loads
# Visual Studio's compiler environment first); tools/run_tests.py cmake does
# all three.
cmake_minimum_required(VERSION 3.25)
# Release unless asked otherwise. It must be set before project(): with MSVC,
# project() itself makes Debug the default (an /Od build, 8 times slower).
if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release (default) or Debug")
endif()
project(rev_bench VERSION 0.13.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RB_WERROR "The core's warnings are errors" ON)
option(RB_LTO "Link-time optimisation where the compiler has it" ON)

# ---- flags ----
if(MSVC)
  # /fp:precise is MSVC's default, and with it MSVC doesn't contract a*b+c
  # into a fused multiply-add (that's /fp:contract, never given here).
  # Release: /O2 and debug info, NDEBUG not defined (as the Makefile's -O3 -g).
  set(CMAKE_C_FLAGS_RELEASE "/O2 /Zi")
  set(RB_WARNINGS /W3)
  set(RB_STRICT /fp:precise)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
else()
  set(CMAKE_C_FLAGS_RELEASE "-O3 -g")
  set(RB_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes)
  set(RB_STRICT -ffp-contract=off)
endif()
add_compile_options(${RB_STRICT})

if(RB_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT RB_HAS_LTO OUTPUT RB_LTO_WHY LANGUAGES C)
  if(RB_HAS_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  else()
    message(STATUS "No link-time optimisation with this compiler")
  endif()
endif()

if(NOT MSVC)
  find_library(RB_LIBM m)
endif()

# ---- the core ----
file(GLOB RB_CORE_SRC CONFIGURE_DEPENDS core/src/*.c)
add_library(rb_core STATIC ${RB_CORE_SRC})
target_include_directories(rb_core PUBLIC core/include PRIVATE core/src)
target_compile_options(rb_core PRIVATE ${RB_WARNINGS})
if(RB_WERROR)
  set_target_properties(rb_core PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endif()
if(RB_LIBM)
  target_link_libraries(rb_core PUBLIC ${RB_LIBM})
endif()

# the core's internals, for the native tools and tests that look inside
add_library(rb_core_internal INTERFACE)
target_include_directories(rb_core_internal INTERFACE core/src)
target_link_libraries(rb_core_internal INTERFACE rb_core)

# ---- native helpers and the research solvers ----
add_library(rb_native STATIC native/wav.c native/csv.c native/fuzz.c)
target_include_directories(rb_native PUBLIC native)
target_link_libraries(rb_native PUBLIC rb_core_internal)
target_compile_options(rb_native PRIVATE ${RB_WARNINGS})

set(RB_ACOUSTIC_SRC
  experiments/acoustics/wave.c experiments/acoustics/euler.c
  experiments/acoustics/coupled_duct.c experiments/acoustics/mixture.c
  experiments/acoustics/gas_graph.c experiments/acoustics/thermal.c
  experiments/acoustics/material.c experiments/acoustics/fixed_transport.c
  experiments/acoustics/junction.c)
add_library(rb_acoustics STATIC ${RB_ACOUSTIC_SRC})
target_include_directories(rb_acoustics PUBLIC experiments/acoustics)
target_link_libraries(rb_acoustics PUBLIC rb_core_internal)
target_compile_options(rb_acoustics PRIVATE ${RB_WARNINGS})

# ---- the programs ----
# rb_cli sound drives the engine through rb.h alone: its object sees no
# core/src (stage 121), so an internal header there doesn't build.
add_library(rb_sound OBJECT native/sound.c)
target_include_directories(rb_sound PRIVATE core/include native)
target_compile_options(rb_sound PRIVATE ${RB_WARNINGS})

add_executable(rb_cli native/rb_cli.c $<TARGET_OBJECTS:rb_sound>)
target_link_libraries(rb_cli PRIVATE rb_native)
target_compile_options(rb_cli PRIVATE ${RB_WARNINGS})

add_executable(rb_bench native/rb_bench.c)
target_link_libraries(rb_bench PRIVATE rb_core_internal)

add_executable(rb_audit native/rb_audit.c)
target_link_libraries(rb_audit PRIVATE rb_core_internal)

add_executable(build_contract_probe tools/build_contract_probe.c)
target_link_libraries(build_contract_probe PRIVATE rb_core_internal)

# ---- the C tests ----
file(GLOB RB_TEST_SRC CONFIGURE_DEPENDS tests/*.c)
add_executable(rb_tests ${RB_TEST_SRC})
target_include_directories(rb_tests PRIVATE tests)
target_link_libraries(rb_tests PRIVATE rb_native rb_acoustics)

enable_testing()
add_test(NAME rb_tests COMMAND rb_tests)
set_tests_properties(rb_tests PROPERTIES ENVIRONMENT "RBT_TMPDIR=${CMAKE_BINARY_DIR}" TIMEOUT 3600)

# ---- the real-time audio host (stage 123) ----
# host/ sees only the public headers (core/include), as rb_cli sound does. miniaudio is
# third-party code: compiled apart, as gnu11, with no warnings and no warnings-as-errors.
# MSVC needs /experimental:c11atomics for <stdatomic.h> (Visual Studio 2022 17.5 or newer).
option(RB_BUILD_HOST "Build the real-time audio host, its tests and tools" ON)
if(RB_BUILD_HOST)
  find_package(Threads REQUIRED)
  add_library(rb_miniaudio OBJECT host/src/miniaudio_impl.c)
  target_include_directories(rb_miniaudio PRIVATE third_party/miniaudio)
  set_target_properties(rb_miniaudio PROPERTIES C_EXTENSIONS ON C_STANDARD 11)
  if(MSVC)
    target_compile_options(rb_miniaudio PRIVATE /W0)
  else()
    target_compile_options(rb_miniaudio PRIVATE -w -O2)
  endif()

  file(GLOB RB_HOST_SRC CONFIGURE_DEPENDS host/src/*.c)
  list(FILTER RB_HOST_SRC EXCLUDE REGEX "(platform_|miniaudio_impl)")
  if(WIN32)
    list(APPEND RB_HOST_SRC host/src/platform_win.c)
  else()
    list(APPEND RB_HOST_SRC host/src/platform_posix.c)
  endif()
  add_library(rb_host STATIC ${RB_HOST_SRC} $<TARGET_OBJECTS:rb_miniaudio>)
  target_include_directories(rb_host PUBLIC host/include PRIVATE host/src)
  target_include_directories(rb_host SYSTEM PRIVATE third_party/miniaudio)
  target_link_libraries(rb_host PUBLIC rb_core Threads::Threads)
  target_compile_options(rb_host PRIVATE ${RB_WARNINGS})
  if(MSVC)
    target_compile_options(rb_host PUBLIC /experimental:c11atomics)
  endif()
  if(RB_WERROR)
    set_target_properties(rb_host PROPERTIES COMPILE_WARNING_AS_ERROR ON)
  endif()
  if(WIN32)
    target_link_libraries(rb_host PUBLIC avrt winmm psapi)
  else()
    target_link_libraries(rb_host PUBLIC ${CMAKE_DL_LIBS})
  endif()

  # the tests look inside the host (host/src): they are the host's own
  file(GLOB RB_HOST_TEST_SRC CONFIGURE_DEPENDS host/tests/*.c)
  list(FILTER RB_HOST_TEST_SRC EXCLUDE REGEX "malloc_wrap")
  add_executable(rb_host_tests ${RB_HOST_TEST_SRC})
  target_include_directories(rb_host_tests PRIVATE host/src host/include)
  target_link_libraries(rb_host_tests PRIVATE rb_host)
  target_compile_options(rb_host_tests PRIVATE ${RB_WARNINGS})
  # GNU ld's --wrap lets the test binary count the C library's allocator calls the render thread
  # makes (not MSVC or macOS: the host's own audit covers them)
  if(NOT MSVC AND NOT APPLE)
    target_sources(rb_host_tests PRIVATE host/tests/malloc_wrap.c)
    target_compile_definitions(rb_host_tests PRIVATE RB_HOST_MALLOC_WRAP=1)
    target_link_options(rb_host_tests PRIVATE LINKER:--wrap=malloc LINKER:--wrap=calloc LINKER:--wrap=realloc LINKER:--wrap=free)
  endif()

  foreach(tool host_render host_stress host_devices)
    add_executable(rb_${tool} host/tools/${tool}.c)
    target_include_directories(rb_${tool} PRIVATE host/src)
    target_link_libraries(rb_${tool} PRIVATE rb_host rb_native)
    target_compile_options(rb_${tool} PRIVATE ${RB_WARNINGS})
  endforeach()

  # the host's tests are labelled `host`, so run_tests.py's host suite can run them alone
  add_test(NAME rb_host_tests COMMAND rb_host_tests)
  set_tests_properties(rb_host_tests PROPERTIES LABELS host TIMEOUT 1200)
endif()
