/*
 * layouts.h - engines built from a family, a cylinder count, a crank and a
 * bank angle (stage 131): see layouts.c and rb.h's "engine layouts".
 */
#ifndef RB_LAYOUTS_H
#define RB_LAYOUTS_H

#include "engine.h"

/* What a layout is: rb.h's RB_FAMILY_* and RB_CRANK_*, and the angle between
 * a V's banks in degrees (0 for an inline or a boxer). */
typedef struct rb_layout {
    int    family;
    int    cylinders;
    int    crank;
    double bank_angle;
} rb_layout;

/* NULL if the layout is one REV-BENCH builds, else a sentence saying why not. */
const char *rb_layout_refusal(const rb_layout *l);

/* Fills `out` with the engine of layout `l` started from preset `preset`
 * (its bore, stroke, cams and the rest that don't depend on the layout; see
 * layouts.c). A preset's own layout gives the preset itself. Returns 0, or
 * -1 (out unchanged) for an unknown preset or a layout refused. */
int rb_layout_desc(int preset, const rb_layout *l, rb_engine_desc *out);

/* Only the crank, banks and firing order of layout `l` on description `d`:
 * cylinders, banks, bank angles, bank, pin, position, firing_order and
 * firing. Returns 0, or -1 (d unchanged) for a layout refused. */
int rb_layout_crank_into(const rb_layout *l, rb_engine_desc *d);

/* The layout each preset is (presets.c's table). */
int rb_preset_layout_of(int preset, rb_layout *out);

#endif /* RB_LAYOUTS_H */
