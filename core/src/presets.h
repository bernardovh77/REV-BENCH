/*
 * presets.h - the engines REV-BENCH comes with (stage 8).
 *
 * Each preset is a full rb_engine_desc: geometry, layout and firing order,
 * valves and cams, intake and exhaust, the crank, the starter and the engine
 * computer's settings. They are generic engines of common designs, not copies
 * of any maker's.
 *
 *   0  i4        2.0 L inline-4, flat crank, firing order 1-3-4-2
 *   1  i6        3.0 L inline-6, firing order 1-5-3-6-2-4
 *   2  v8-cross  5.0 L 90-degree V8, cross-plane crank, 1-8-4-3-6-5-7-2
 *   3  v8-flat   4.5 L 90-degree V8, flat-plane crank, 1-6-5-8-7-4-3-2
 *   4  boxer4    2.0 L flat-4 (boxer), firing order 1-3-2-4
 *
 * The names of these functions that the browser uses are in rb.h.
 */
#ifndef RB_PRESETS_H
#define RB_PRESETS_H

#include "engine.h"

/* Fills d with preset number `preset` (0..rb_preset_count()-1). Returns 0,
 * or -1 for an unknown preset (d unchanged). */
int rb_preset_desc(int preset, rb_engine_desc *d);

#endif /* RB_PRESETS_H */
