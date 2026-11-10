# REV-BENCH's Node-API add-on (stage 124): rb_native.node.
#
# It wraps the real-time host (host/) and the core's metadata (core/) for
# Node, and implements app/src/shared/addon.ts. Built by cmake-js, which
# gives CMAKE_JS_INC (node-api-headers' include folder), and on Windows
# CMAKE_JS_NODELIB_DEF and CMAKE_JS_NODELIB_TARGET (the definition file of
# node.exe's Node-API functions, and where to make node.lib from it):
#
#   node <tools>/node_modules/cmake-js/bin/cmake-js build -d app/native -O <out>
#
# tools/run_tests.py addon does this (MSVC through tools/msvc_env.py, Ninja),
# installing cmake-js 8.0.0 and node-api-headers 1.9.0 outside the repo.
#
# The core, the host and miniaudio come from the repo's own CMakeLists.txt, as
# targets (rb_host and rb_core), so the add-on is built from the very sources,
# warnings and flags the host's tests are: add_subdirectory(.. EXCLUDE_FROM_ALL)
# builds only what the add-on needs, never the tests and tools. The add-on's
# own code is built here, with the same strict double arithmetic (no fused
# multiply-add, no fast-math).
#
# One .node loads in plain Node and in Electron: Node-API only (no V8, no
# NAN), and on Windows the node.exe import is delay-loaded and answered with
# the running executable (src/rbn_win_delay_load.c).
cmake_minimum_required(VERSION 3.25)
if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release (default) or Debug")
endif()
project(rb_native_addon LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# the static libraries end up inside a shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(RB_ADDON_LTO "Link-time optimisation where the compiler has it" ON)
option(RB_WERROR "The core's and the add-on's warnings are errors" ON)

if(NOT CMAKE_JS_INC)
  message(FATAL_ERROR "Build this with cmake-js (it gives CMAKE_JS_INC): see the top of this file")
endif()

# ---- flags: the same as the root CMakeLists.txt ----
if(MSVC)
  set(RB_ADDON_WARNINGS /W3)
  set(RB_STRICT /fp:precise)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
else()
  set(RB_ADDON_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes)
  set(RB_STRICT -ffp-contract=off)
endif()
add_compile_options(${RB_STRICT})

# Link-time optimisation: the root's libraries are built with it, so the add-on
# must be linked with it (gcc cannot link their objects without)
if(RB_ADDON_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT RB_ADDON_HAS_LTO OUTPUT RB_ADDON_LTO_WHY LANGUAGES C)
  if(RB_ADDON_HAS_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  else()
    set(RB_LTO OFF CACHE BOOL "" FORCE)
    message(STATUS "No link-time optimisation with this compiler")
  endif()
else()
  set(RB_LTO OFF CACHE BOOL "" FORCE)
endif()

# ---- the core and the host, from the repo's root ----
set(RB_BUILD_HOST ON CACHE BOOL "" FORCE)
get_filename_component(RB_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_subdirectory("${RB_REPO_ROOT}" rb_root EXCLUDE_FROM_ALL)

# ---- the add-on ----
# The target is not called rb_native: the root has a library of that name, so
# the import library and the debug info of the .node are named apart too.
file(GLOB RBN_SRC CONFIGURE_DEPENDS src/*.c)
if(NOT WIN32)
  list(FILTER RBN_SRC EXCLUDE REGEX "rbn_win_")
endif()
add_library(rb_native_addon SHARED ${RBN_SRC})
set_target_properties(rb_native_addon PROPERTIES
  OUTPUT_NAME rb_native PREFIX "" SUFFIX ".node"
  ARCHIVE_OUTPUT_NAME rb_native_addon PDB_NAME rb_native_addon
  C_VISIBILITY_PRESET hidden
  # always <build folder>/rb_native.node, whatever cmake-js asked for (it differs per system)
  LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}>"
  RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}>")
target_include_directories(rb_native_addon PRIVATE src)
# node-api-headers' folder is a system include: its code is not ours to warn about
target_include_directories(rb_native_addon SYSTEM PRIVATE ${CMAKE_JS_INC})
target_compile_options(rb_native_addon PRIVATE ${RB_ADDON_WARNINGS})
if(RB_WERROR)
  set_target_properties(rb_native_addon PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endif()
target_link_libraries(rb_native_addon PRIVATE rb_host rb_core)

if(MSVC)
  # node.lib, made from the definition file of node.exe's Node-API functions
  if(CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET}
                            /machine:${CMAKE_C_COMPILER_ARCHITECTURE_ID}
                    RESULT_VARIABLE RBN_LIB_RESULT)
    if(NOT RBN_LIB_RESULT EQUAL 0)
      message(FATAL_ERROR "could not make node.lib from ${CMAKE_JS_NODELIB_DEF}")
    endif()
  endif()
  # the import of node.exe is delay-loaded, and answered by the running executable
  target_link_libraries(rb_native_addon PRIVATE ${CMAKE_JS_LIB} delayimp)
  target_link_options(rb_native_addon PRIVATE /DELAYLOAD:node.exe)
elseif(APPLE)
  # the Node-API functions are the executable's, found when the add-on loads
  target_link_options(rb_native_addon PRIVATE -undefined dynamic_lookup)
else()
  # the static libraries' own symbols stay inside the add-on
  target_link_options(rb_native_addon PRIVATE LINKER:--exclude-libs,ALL)
endif()
