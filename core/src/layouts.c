/*
 * layouts.c - engines built from a family, a cylinder count, a crank and a
 * bank angle (stage 131).
 *
 * The crank. Every cylinder has its own pin in the description, so any
 * firing can be built: an even-firing engine is made from the times its
 * cylinders should fire, each pin worked out so the piston is at TDC then.
 * Cylinders count from the front; on a V or a boxer the even ones (from 0)
 * are on bank 0 and the odd ones on bank 1, a row's two cylinders side by
 * side, as presets.c numbers them. Bank 0 leans at +angle/2 (a boxer's at
 * +90), bank 1 at -angle/2, so with a pin shared, bank 1's piston reaches
 * TDC `angle` degrees before bank 0's.
 *
 * - Inline, even: the row's firing order (ROW_ORDER) every 720/n degrees.
 * - V, even: two inline engines of n/2, bank 1's each cylinder 720/n degrees
 *   before its row partner, so each bank fires evenly too. At a bank angle
 *   of 720/n (a V12 at 60, a V10 at 72) or a V8's 90 the pins come out
 *   shared; at others they split (a 60-degree V6's by 60).
 * - V, common pin: the inline crank of one bank, its two cylinders on each
 *   pin; bank 1's cylinder fires on whichever turn of the crank makes the
 *   firing most even (all 2^(n/2) choices are tried; ties go to the turn
 *   just before its partner).
 * - V8, cross-plane: the cross-plane V8 preset's pins and firing order.
 * - Boxer, even: a row's two cylinders on pins 180 apart reach TDC
 *   together, one firing, the other 360 degrees on. The rows fire in the
 *   inline order; with an odd number of rows the banks take turns row by
 *   row (the six), with an even number bank 0 fires its half first (the
 *   four's 1-3-2-4).
 * - Inline twins: 180 or 270 degrees between the two pins.
 *
 * The rest of the engine. What doesn't depend on the layout comes from the
 * preset it starts from (bore, stroke, rod, compression, valves and cams,
 * the burn, the walls, the friction, the rev limit, cranking, runner and
 * header sizes, the kinds of muffler and pipes). What does is worked out:
 * - the exhaust's grouping: headers into one collector, or one per bank (a
 *   V of 4 or more, a boxer six) and the inline six's front and rear three,
 *   each fed by cylinders firing evenly; two systems for a V of 6 or more and
 *   a boxer six. Each header runs from its port to its collector at the back
 *   of its group, so they differ in length; a boxer four's one collector
 *   has the far bank's headers 30 cm longer, crossing under the engine;
 * - sizes that follow the air through it, scaled from the preset by
 *   displacement (per exhaust system for the exhaust): the throttle and the
 *   snorkel by its 0.35th power, the air box by its square root, the plenum
 *   in proportion; the pipes' bores by the square root of the flow, the
 *   catalyst's, mufflers' and junction's areas and volumes in proportion;
 * - the running set, from the displacement V (litres) and count n, fitted to
 *   the five presets (docs/stage-131.md): flywheel 0.08 V kg m^2 (the four's
 *   per litre: with more a small engine came back from its limiter slowly),
 *   starter 200 N m times the larger of (V/2)^0.8 and one cylinder's
 *   displacement over half a litre, accessories 800 W sqrt(V/2), idle speed
 *   1100 rpm for a single, 1000 for a twin, 900 a triple, then 800 less 25
 *   per cylinder over four (600 at least), the idle air valve 47 mm^2
 *   (V/2)^0.8 and the idle gains so that gain times valve area over the
 *   flywheel's inertia is what the four's is: the idle's loop as quick on
 *   any engine.
 */
#include "layouts.h"
#include "presets.h"

#include "rb.h"

#include <math.h>
#include <string.h>

static const double PI_ = 3.14159265358979323846;

/* Each inline engine's firing order, rows counted from 0: even firing, the
 * usual ones (an I3's 1-2-3, an I4's 1-3-4-2, an I5's 1-2-4-5-3, an I6's
 * 1-5-3-6-2-4); 7 and 8 rows only make the V14's and the V16's banks (the
 * eight is the straight eight's 1-6-2-5-8-3-7-4, the seven steps by three). */
static const int ROW_ORDER[9][8] = {
    { 0 },
    { 0 },
    { 0, 1 },
    { 0, 1, 2 },
    { 0, 2, 3, 1 },
    { 0, 1, 3, 4, 2 },
    { 0, 4, 2, 5, 1, 3 },
    { 0, 3, 6, 2, 5, 1, 4 },
    { 0, 5, 1, 4, 7, 2, 6, 3 },
};

/* The cross-plane V8 preset's crank (presets.c, v8_cross). */
static const double CROSS_PIN[8] = { 0.0, 0.0, 90.0, 90.0, 270.0, 270.0, 180.0, 180.0 };
static const int CROSS_ORDER[8] = { 0, 7, 3, 2, 5, 4, 6, 1 };

static const char *const FAMILY_NAMES[RB_FAMILY_COUNT] = { "Inline", "V", "Boxer" };
static const char *const CRANK_NAMES[RB_CRANK_COUNT] = {
    "Even firing", "Common pins", "Cross-plane", "180-degree twin", "270-degree twin",
};

const char *rb_family_name(int family)
{
    return (family >= 0 && family < RB_FAMILY_COUNT) ? FAMILY_NAMES[family] : NULL;
}

const char *rb_crank_name(int crank)
{
    return (crank >= 0 && crank < RB_CRANK_COUNT) ? CRANK_NAMES[crank] : NULL;
}

/* An angle in [0, 360). */
static double turn360(double a)
{
    const double m = fmod(a, 360.0);
    return (m < 0.0) ? m + 360.0 : m;
}

/* An angle in [0, 720). */
static double turn720(double a)
{
    const double m = fmod(a, 720.0);
    return (m < 0.0) ? m + 720.0 : m;
}

/* The banks: 1 for an inline, else 2 with their angles. */
static void banks_of(const rb_layout *l, rb_engine_desc *d)
{
    if (l->family == RB_FAMILY_INLINE) {
        d->banks = 1;
        d->bank_angle[0] = d->bank_angle[1] = 0.0;
        return;
    }
    const double half = (l->family == RB_FAMILY_BOXER) ? 90.0 : 0.5 * l->bank_angle;
    d->banks = 2;
    d->bank_angle[0] = half;
    d->bank_angle[1] = -half;
}

/* Gives each cylinder the pin that puts it at TDC `target[i]` degrees after
 * cylinder 1's (target[0] is 0), and the firing order the targets' order. */
static void pins_from_targets(rb_engine_desc *d, const double *target)
{
    const int n = d->cylinders;
    const double base = d->bank_angle[d->bank[0]];
    int order[RB_MAX_CYLINDERS];
    for (int i = 0; i < n; i++) {
        d->pin[i] = turn360(d->bank_angle[d->bank[i]] - base - target[i]);
        order[i] = i;
    }
    /* insertion sort by target: n is at most 16 */
    for (int a = 1; a < n; a++) {
        const int c = order[a];
        int b = a - 1;
        while (b >= 0 && target[order[b]] > target[c]) {
            order[b + 1] = order[b];
            b--;
        }
        order[b + 1] = c;
    }
    for (int i = 0; i < n; i++) d->firing_order[i] = order[i];
}

/* The smallest gap (deg) between successive firings of a 720-degree cycle. */
static double smallest_gap(const double *target, int n)
{
    double sorted[RB_MAX_CYLINDERS];
    for (int i = 0; i < n; i++) sorted[i] = target[i];
    for (int a = 1; a < n; a++) {
        const double t = sorted[a];
        int b = a - 1;
        while (b >= 0 && sorted[b] > t) {
            sorted[b + 1] = sorted[b];
            b--;
        }
        sorted[b + 1] = t;
    }
    double gap = 720.0 - sorted[n - 1] + sorted[0];
    for (int i = 1; i < n; i++)
        if (sorted[i] - sorted[i - 1] < gap) gap = sorted[i] - sorted[i - 1];
    return gap;
}

/* The firing times of a common-pin V: bank 0 as the inline crank of one
 * bank fires, bank 1 on the turn that fires most evenly. */
static void common_pin_targets(const rb_layout *l, double *target)
{
    const int m = l->cylinders / 2;
    const double step = 720.0 / m;
    double best[RB_MAX_CYLINDERS], trial[RB_MAX_CYLINDERS], best_gap = -1.0;
    for (int j = 0; j < m; j++) trial[2 * ROW_ORDER[m][j]] = j * step;
    for (int mask = 0; mask < (1 << m); mask++) {
        for (int r = 0; r < m; r++) {
            const double just_before = turn720(trial[2 * r] - l->bank_angle);
            trial[2 * r + 1] = (mask >> r & 1) ? turn720(just_before + 360.0) : just_before;
        }
        const double gap = smallest_gap(trial, l->cylinders);
        if (gap > best_gap + 1e-9) {
            best_gap = gap;
            memcpy(best, trial, sizeof(double) * (size_t)l->cylinders);
        }
    }
    memcpy(target, best, sizeof(double) * (size_t)l->cylinders);
}

/* Where each cylinder sits along the crank (for drawing it): rows a bore
 * and a gap apart, a V's bank 1 a rod's big end behind its partner, a
 * boxer's half a bore. */
static void positions(const rb_layout *l, rb_engine_desc *d)
{
    for (int i = 0; i < d->cylinders; i++) {
        if (l->family == RB_FAMILY_INLINE) {
            d->position[i] = i * (d->bore + 0.010);
        } else if (l->family == RB_FAMILY_V) {
            d->position[i] = (i / 2) * (d->bore + 0.019) + (i % 2) * 0.24 * d->bore;
        } else {
            d->position[i] = (i / 2) * (d->bore + 0.029) + (i % 2) * 0.48 * d->bore;
        }
    }
}

int rb_layout_crank_into(const rb_layout *l, rb_engine_desc *d)
{
    if (!l || !d) return -1;
    rb_engine_desc x = *d;
    const int n = l->cylinders;
    if (n < 1 || n > RB_MAX_CYLINDERS) return -1;
    x.cylinders = n;
    banks_of(l, &x);
    for (int i = 0; i < n; i++) x.bank[i] = (x.banks == 2) ? i % 2 : 0;
    positions(l, &x);

    double target[RB_MAX_CYLINDERS] = { 0.0 };
    const double step = 720.0 / n;
    if (l->family == RB_FAMILY_INLINE && l->crank == RB_CRANK_EVEN) {
        if (n > 8) return -1;
        for (int j = 0; j < n; j++) target[ROW_ORDER[n][j]] = j * step;
        pins_from_targets(&x, target);
    } else if (l->family == RB_FAMILY_INLINE && (l->crank == RB_CRANK_TWIN_180 || l->crank == RB_CRANK_TWIN_270)) {
        if (n != 2) return -1;
        target[1] = (l->crank == RB_CRANK_TWIN_180) ? 180.0 : 270.0;
        pins_from_targets(&x, target);
    } else if (l->family == RB_FAMILY_V && l->crank == RB_CRANK_EVEN) {
        const int m = n / 2;
        if (n % 2 || m > 8) return -1;
        for (int j = 0; j < m; j++) {
            const int r = ROW_ORDER[m][j];
            target[2 * r] = j * 2.0 * step;
            target[2 * r + 1] = turn720(j * 2.0 * step - step);
        }
        pins_from_targets(&x, target);
    } else if (l->family == RB_FAMILY_V && l->crank == RB_CRANK_COMMON_PIN) {
        if (n % 2 || n / 2 > 8) return -1;
        common_pin_targets(l, target);
        pins_from_targets(&x, target);
    } else if (l->family == RB_FAMILY_V && l->crank == RB_CRANK_CROSS_PLANE) {
        if (n != 8) return -1;
        for (int i = 0; i < 8; i++) {
            x.pin[i] = CROSS_PIN[i];
            x.firing_order[i] = CROSS_ORDER[i];
        }
    } else if (l->family == RB_FAMILY_BOXER && l->crank == RB_CRANK_EVEN) {
        const int m = n / 2;
        if (n % 2 || m > 8) return -1;
        for (int j = 0; j < m; j++) {
            const int r = ROW_ORDER[m][j];
            const int first = (m % 2 == 1 && j % 2 == 1) ? 1 : 0;   /* the bank firing first */
            target[2 * r + first] = j * step;
            target[2 * r + 1 - first] = j * step + 360.0;
        }
        pins_from_targets(&x, target);
    } else {
        return -1;
    }
    if (rb_engine_layout(&x) != 0) return -1;
    /* Each cylinder must fire when it was meant to: rb_engine_layout takes
     * the first TDC after the last firing, which a gap over 360 degrees
     * would move a turn early. */
    if (l->crank != RB_CRANK_CROSS_PLANE)
        for (int i = 0; i < n; i++)
            if (!(fabs(x.firing[i] - target[i]) < 1e-6)) return -1;
    *d = x;
    return 0;
}

/* ---- what's offered ---- */

static int is_even(int n)
{
    return n % 2 == 0;
}

const char *rb_layout_refusal(const rb_layout *l)
{
    if (!l) return "no layout";
    const int n = l->cylinders;
    if (l->family < 0 || l->family >= RB_FAMILY_COUNT) return "an unknown family of engine";
    if (l->crank < 0 || l->crank >= RB_CRANK_COUNT) return "an unknown crank";
    if (!isfinite(l->bank_angle)) return "a bank angle that isn't a number";
    switch (l->family) {
    case RB_FAMILY_INLINE:
        if (n == 7 || n == 8) return "an inline engine of 7 or 8 cylinders isn't offered yet (an 8 is planned)";
        if (n < 1 || n > 6) return "an inline engine has 1 to 6 cylinders";
        if (l->bank_angle != 0.0) return "an inline engine has no bank angle (0)";
        if (l->crank == RB_CRANK_TWIN_180 || l->crank == RB_CRANK_TWIN_270) {
            if (n != 2) return "a 180- or 270-degree crank is a twin's";
        } else if (l->crank != RB_CRANK_EVEN) {
            return "an inline engine's crank is even firing (or a twin's 180 or 270 degrees)";
        }
        break;
    case RB_FAMILY_V:
        if (n < 2 || n > 16 || !is_even(n)) return "a V has an even number of cylinders, 2 to 16";
        if (!(l->bank_angle >= 30.0 && l->bank_angle <= 120.0)) return "a V's banks are 30 to 120 degrees apart";
        if (l->crank == RB_CRANK_CROSS_PLANE && n != 8) return "a cross-plane crank is a V8's";
        if (l->crank != RB_CRANK_EVEN && l->crank != RB_CRANK_COMMON_PIN && l->crank != RB_CRANK_CROSS_PLANE)
            return "a V's crank is even firing, common pins or (a V8's) cross-plane";
        break;
    case RB_FAMILY_BOXER:
        if (n == 8) return "a boxer eight isn't offered yet (it is planned)";
        if (n != 2 && n != 4 && n != 6) return "a boxer has 2, 4 or 6 cylinders";
        if (l->bank_angle != 0.0) return "a boxer's banks are always 180 degrees apart (bank angle 0)";
        if (l->crank != RB_CRANK_EVEN) return "a boxer's crank is even firing";
        break;
    default:
        return "an unknown family of engine";
    }
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    if (rb_layout_crank_into(l, &d) != 0) {
        if (l->crank == RB_CRANK_COMMON_PIN) return "on common pins at this bank angle two cylinders would fire at once";
        return "this crank can't fire each cylinder once a cycle at that bank angle";
    }
    return NULL;
}

int rb_layout_check(int family, int cylinders, int crank, double bank_angle)
{
    const rb_layout l = { family, cylinders, crank, bank_angle };
    if (family < 0 || family >= RB_FAMILY_COUNT || crank < 0 || crank >= RB_CRANK_COUNT) return RB_ERR_ID;
    return rb_layout_refusal(&l) ? RB_ERR_VALUE : RB_OK;
}

const char *rb_layout_why(int family, int cylinders, int crank, double bank_angle)
{
    const rb_layout l = { family, cylinders, crank, bank_angle };
    return rb_layout_refusal(&l);
}

int rb_layout_usual_crank(int family, int cylinders)
{
    int crank = RB_CRANK_EVEN;
    if (family == RB_FAMILY_V && cylinders == 2) crank = RB_CRANK_COMMON_PIN;
    if (family == RB_FAMILY_V && cylinders == 8) crank = RB_CRANK_CROSS_PLANE;
    const double angle = (family == RB_FAMILY_V) ? rb_layout_angle(family, cylinders, crank, RB_ANGLE_USUAL) : 0.0;
    return rb_layout_check(family, cylinders, crank, isnan(angle) ? 90.0 : angle) == RB_OK ? crank : RB_ERR_VALUE;
}

double rb_layout_angle(int family, int cylinders, int crank, int which)
{
    if (which < RB_ANGLE_MIN || which > RB_ANGLE_USUAL) return NAN;
    if (family != RB_FAMILY_V) return rb_layout_check(family, cylinders, crank, 0.0) == RB_OK ? 0.0 : NAN;
    /* the usual angles: a V-twin's, V4's and V8's 90, a V6's and V12's 60,
     * a V10's 72, a V14's and V16's shared-pin even firing (720/n) */
    double usual = 90.0;
    if (cylinders == 6 || cylinders == 12) usual = 60.0;
    if (cylinders == 10) usual = 72.0;
    if (cylinders == 14 || cylinders == 16) usual = 720.0 / cylinders;
    if (rb_layout_check(family, cylinders, crank, usual) != RB_OK) return NAN;
    if (which == RB_ANGLE_MIN) return 30.0;
    if (which == RB_ANGLE_MAX) return 120.0;
    return usual;
}

int rb_layout_quality(int family, int cylinders, int crank)
{
    const double angle = rb_layout_angle(family, cylinders, crank, RB_ANGLE_USUAL);
    if (isnan(angle)) return RB_ERR_VALUE;
    /* the presets' tiers by count (stage 118, 127): lean is the cheapest */
    if (cylinders <= 4) return RB_QUALITY_BALANCED;
    if (cylinders <= 6) return RB_QUALITY_LOW;
    return RB_QUALITY_LEAN;
}

/* ---- the rest of the engine ---- */

/* m^3, all the cylinders together. */
static double displacement(const rb_engine_desc *d)
{
    return d->cylinders * 0.25 * PI_ * d->bore * d->bore * d->stroke;
}

/* The headers: which collector each joins, how long each is, and how many
 * exhaust systems. */
static void exhaust_grouping(const rb_layout *l, rb_engine_desc *d)
{
    const int n = d->cylinders;
    int collectors = 1;
    if (l->family == RB_FAMILY_V && n >= 4) collectors = 2;
    if (l->family == RB_FAMILY_BOXER && n == 6) collectors = 2;
    if (l->family == RB_FAMILY_INLINE && n == 6) collectors = 2;
    for (int i = 0; i < n; i++) {
        if (collectors == 1) d->collector[i] = 0;
        else if (l->family == RB_FAMILY_INLINE) d->collector[i] = (i < n / 2) ? 0 : 1;
        else d->collector[i] = d->bank[i];
    }
    int systems = 1;
    if (collectors == 2 && ((l->family == RB_FAMILY_V && n >= 6) || l->family == RB_FAMILY_BOXER)) systems = 2;
    d->exhaust_systems = systems;

    /* Each collector at the back of its group, 70 % of the way along it: a
     * header is 28 cm plus its port's distance from there. */
    for (int c = 0; c < collectors; c++) {
        double front = 1e9, back = -1e9;
        for (int i = 0; i < n; i++) {
            if (d->collector[i] != c) continue;
            if (d->position[i] < front) front = d->position[i];
            if (d->position[i] > back) back = d->position[i];
        }
        const double at = front + 0.7 * (back - front);
        for (int i = 0; i < n; i++)
            if (d->collector[i] == c) d->header_length[i] = 0.28 + fabs(d->position[i] - at);
    }
    if (l->family == RB_FAMILY_BOXER && collectors == 1 && n >= 4)
        for (int i = 0; i < n; i++)
            if (d->bank[i] == 1) d->header_length[i] += 0.30;   /* crossing under the engine */

    /* A shared junction (the cross-plane V8's X) only with two systems. */
    if (d->exhaust_systems != 2) {
        d->junction_volume = 0.0;
        d->junction_before_center = 0;
    }
}

/* The sizes that follow the air, scaled from the preset `b` by
 * displacement (per exhaust system for the exhaust). */
static void scale_flows(const rb_engine_desc *b, rb_engine_desc *d)
{
    const double v = displacement(d) / displacement(b);
    const double q = v * b->exhaust_systems / d->exhaust_systems;
    d->throttle.diameter = b->throttle.diameter * pow(v, 0.35);
    d->throttle.shaft = d->throttle.diameter > 0.07 ? 0.010 : 0.008;
    d->throttle.leak_area = b->throttle.leak_area * pow(v, 0.7);
    d->snorkel_diameter = b->snorkel_diameter * pow(v, 0.35);
    d->airbox_volume = b->airbox_volume * sqrt(v);
    d->plenum_volume = b->plenum_volume * v;
    d->exhaust_pipe_diameter = b->exhaust_pipe_diameter * sqrt(q);
    d->tailpipe_diameter = b->tailpipe_diameter * sqrt(q);
    d->center_diameter = b->center_diameter * sqrt(q);
    d->center_volume = b->center_volume * q;
    d->catalyst_area = b->catalyst_area * q;
    d->muffler_area = b->muffler_area * q;
    d->muffler_volume = b->muffler_volume * q;
    if (d->junction_volume > 0.0) d->junction_volume = b->junction_volume * v;
}

/* The flywheel, starter, accessories and the idle, from the size. */
static void running_set(rb_engine_desc *d)
{
    const int n = d->cylinders;
    const double litres = displacement(d) * 1000.0;
    const double cylinder = litres / n;
    d->inertia = 0.08 * litres;
    d->starter_torque = 200.0 * fmax(pow(litres / 2.0, 0.8), cylinder / 0.5);
    d->starter_free_rpm = 250.0;
    d->accessory_power = 800.0 * sqrt(litres / 2.0);
    if (n == 1) d->idle_rpm = 1100.0;
    else if (n == 2) d->idle_rpm = 1000.0;
    else if (n == 3) d->idle_rpm = 900.0;
    else d->idle_rpm = fmax(600.0, 800.0 - 25.0 * (n - 4));
    d->idle_valve_area = 47e-6 * pow(litres / 2.0, 0.8);
    /* the four's: 2.4e-4 per rpm on a 47 mm^2 valve and a 0.16 kg m^2 crank */
    const double loop = 2.4e-4 * 47e-6 / 0.16;
    d->idle_gain_p = loop * d->inertia / d->idle_valve_area;
    d->idle_gain_i = loop * d->inertia / d->idle_valve_area;
}

static int same_layout(const rb_layout *a, const rb_layout *b)
{
    return a->family == b->family && a->cylinders == b->cylinders && a->crank == b->crank
        && fabs(a->bank_angle - b->bank_angle) < 1e-9;
}

int rb_layout_desc(int preset, const rb_layout *l, rb_engine_desc *out)
{
    rb_engine_desc base;
    rb_layout own;
    if (!l || !out || rb_preset_desc(preset, &base) != 0 || rb_preset_layout_of(preset, &own) != 0) return -1;
    if (rb_layout_refusal(l)) return -1;
    if (same_layout(l, &own)) {
        *out = base;
        return 0;
    }
    rb_engine_desc d = base;
    if (rb_layout_crank_into(l, &d) != 0) return -1;
    exhaust_grouping(l, &d);
    scale_flows(&base, &d);
    running_set(&d);
    if (rb_engine_desc_check(&d) != 0) return -1;
    *out = d;
    return 0;
}
