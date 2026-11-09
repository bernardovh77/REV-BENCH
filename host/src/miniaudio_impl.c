/*
 * miniaudio_impl.c - the one place miniaudio's implementation is compiled
 * (stage 123). It is third-party code (third_party/miniaudio, version and
 * licence in NOTICE.md there), built apart from the host's own sources
 * without the project's warnings and without the pieces the host doesn't
 * use: only the device layer and its sample-rate converter stay
 * (ma_config.h).
 */
#include "ma_config.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
