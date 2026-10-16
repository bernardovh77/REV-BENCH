/*
 * throttle.h - the butterfly throttle: a round plate on a shaft across the
 * throttle bore. The pedal turns the plate, which opens two crescent-shaped
 * gaps, one on each side of the shaft.
 *
 * Seen along the bore, the plate is an ellipse. Closed, it is tilted by
 * closed_angle (so it can wedge shut) and fills the whole bore. As it turns
 * towards 90 degrees it looks thinner and thinner, until at wide-open
 * throttle it is edge-on and only the shaft is left in the way:
 *
 *        .-'''-.            .-'''-.            .-'''-.
 *      /#########\        /         \        /         \
 *     |###########|      |(#########)|      |===========|
 *      \#########/        \         /        \         /
 *        '-...-'            '-...-'            '-...-'
 *        closed       part open: gaps     wide open: the
 *                     above and below     shaft is left
 *
 * With R the bore radius, psi the plate angle (closed_angle when shut, 90
 * degrees wide open), b = R cos(psi) / cos(closed_angle) the plate ellipse's
 * half-height, and the shaft a strip of half-width h = d / 2 across the bore:
 *
 *     open area = circle - (ellipse + strip) = pi R^2 - pi R b - S_c + S_e
 *
 * S_c and S_e are the areas of the strip inside the circle and inside the
 * ellipse (the strip overlaps the plate, so that part must not be removed
 * twice). Heywood gives an equivalent formula; this one is derived from the
 * picture above and checked against a direct numerical integration in the
 * tests. The plate's own thickness is ignored.
 *
 * The pedal position (0..1) turns the plate linearly from closed_angle to 90
 * degrees. An idling engine needs only a tiny part of the bore's area, so it
 * idles with the throttle a few % open.
 */
#ifndef RB_THROTTLE_H
#define RB_THROTTLE_H

typedef struct rb_throttle {
    double diameter;       /* m, bore of the throttle body                        */
    double shaft;          /* m, shaft diameter                                   */
    double closed_angle;   /* degrees from the plane across the bore when shut    */
    double cd;             /* discharge coefficient of the open area              */
    double leak_area;      /* m^2, effective area that is always open (gaps, idle air bypass) */
} rb_throttle;

/* 0 if every field is sensible, -1 otherwise. */
int    rb_throttle_check(const rb_throttle *t);

/* Plate angle (degrees) at pedal position 0..1 (clamped; NaN counts as 0). */
double rb_throttle_angle(const rb_throttle *t, double position);

/* Geometric open area (m^2) at a plate angle in degrees. */
double rb_throttle_open_area(const rb_throttle *t, double angle_deg);

/* Effective flow area (m^2): cd * open area + leak_area, at pedal position 0..1. */
double rb_throttle_area(const rb_throttle *t, double position);

#endif /* RB_THROTTLE_H */
