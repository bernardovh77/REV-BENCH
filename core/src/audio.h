/*
 * audio.h - from the gas to the sound (stage 11): the microphone, and the
 * chain that takes its signal down to the audio rate.
 *
 * The microphone. The engine's sound is the gas leaving its tailpipes. An
 * open pipe end small against the wavelengths it radiates (a 50 mm pipe
 * against 0.3-3 m) radiates like a pulsating sphere: a monopole, whose
 * strength is the volume flow Q out of it, the gas the outside air has to
 * make room for. In the open, at a distance r (Kinsler et al., chapter 7),
 *
 *     p(r, t) = rho0 / (4 pi r) dQ/dt,     Q = mdot / rho
 *
 * rho0 the outside air's density, mdot and rho the flow out of the pipe and
 * the density of the gas leaving it (the outside air's, when the pipe draws
 * air in). The sound takes r / c to arrive; with one source that's only a
 * delay, so it's left out. Several tailpipes at the same place add up.
 *
 * The chain, at each audio frame:
 *
 *   the microphone, every physics step (RB_FULL_SCALE_PA is an output of 1)
 *     -> decimator: an anti-alias filter, then every `factor`-th value
 *     -> DC blocker -> gain -> soft limiter -> the sample
 *
 * The decimator's filter is a windowed sinc, a linear-phase FIR (Oppenheim
 * and Schafer, chapter 7): flat to 0.417 of the audio rate (20 kHz at
 * 48 kHz) and 100 dB down from 0.583 of it (28 kHz) up to the physics rate's
 * Nyquist, so nothing the physics makes above 28 kHz folds back into the
 * audible band. Between 24 and 28 kHz it only folds to 20-24 kHz.
 */
#ifndef RB_AUDIO_H
#define RB_AUDIO_H

#define RB_FULL_SCALE_PA  40.0   /* Pa at the microphone for an output of 1:
                                    126 dB SPL. The loudest preset peaks at 19 Pa
                                    (the flat-plane V8, 3000 rpm, wide open)    */
#define RB_MIC_DISTANCE   1.0    /* m from the tailpipes, by default            */
#define RB_DC_CUTOFF      10.0   /* Hz: the DC blocker's corner                 */
#define RB_LIMIT_KNEE     0.8    /* the soft limiter leaves samples below this  */

/* ---- the microphone ---- */

typedef struct rb_mic {
    double distance;   /* m from the sources                                     */
    double q;          /* m^3/s: their volume flow at the last step              */
    int    primed;     /* q holds a flow (the first step can't have a derivative) */
} rb_mic;

void   rb_mic_init(rb_mic *m, double distance);

/* Forgets the last flow: the next step hears nothing, so a source that jumps
 * (an engine rebuilt) doesn't click. */
void   rb_mic_reset(rb_mic *m);

/* The pressure (Pa) the microphone hears over a step of dt (s), given the
 * sources' volume flow q (m^3/s) now and the outside air's density rho0. */
double rb_mic_hear(rb_mic *m, double q, double rho0, double dt);

/* ---- the decimator ---- */

#define RB_DECIMATOR_MAX_FACTOR 32     /* RB_MAX_OVERSAMPLE                      */
#define RB_DECIMATOR_MAX_TAPS   1245   /* the filter's length at that factor     */

typedef struct rb_decimator {
    int    factor;                          /* physics steps per audio frame      */
    int    taps;                            /* the filter's length (odd)          */
    int    pos;                             /* where the next input goes          */
    double h[RB_DECIMATOR_MAX_TAPS];        /* the filter, symmetric              */
    double hist[2 * RB_DECIMATOR_MAX_TAPS]; /* the last `taps` inputs, kept twice
                                               so they're always in one piece     */
} rb_decimator;

/* Designs the filter for `factor` (1..RB_DECIMATOR_MAX_FACTOR; 1 passes the
 * signal through) and clears the history. Returns 0, or -1 for a bad factor. */
int    rb_decimator_init(rb_decimator *d, int factor);

/* Clears the history (the filter stays). */
void   rb_decimator_reset(rb_decimator *d);

/* One input, at the physics rate. */
void   rb_decimator_push(rb_decimator *d, double x);

/* The filter's output now. Call it once every `factor` pushes: that's the
 * signal at the audio rate, (taps - 1) / 2 physics steps late. */
double rb_decimator_out(const rb_decimator *d);

/* ---- the output stage ---- */

/* A one-pole DC blocker, y[n] = x[n] - x[n-1] + r y[n-1] (Smith, "DC
 * blocker"): a high-pass with its corner at `cutoff` Hz. */
typedef struct rb_dc_blocker {
    double r, x1, y1;
} rb_dc_blocker;

void   rb_dc_blocker_init(rb_dc_blocker *b, double cutoff, double sample_rate);
void   rb_dc_blocker_reset(rb_dc_blocker *b);
double rb_dc_blocker_step(rb_dc_blocker *b, double x);

/* The soft limiter: samples within RB_LIMIT_KNEE pass untouched; above it
 * they bend smoothly towards 1, never beyond it,
 *
 *     y = knee + (1 - knee) tanh((|x| - knee) / (1 - knee))
 *
 * with x's sign. Its slope is 1 at the knee, so the bend makes no edge. */
double rb_soft_limit(double x);

#endif /* RB_AUDIO_H */
