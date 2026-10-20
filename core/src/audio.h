/*
 * audio.h - from the gas to the sound: the listener (stages 11 and 12), and
 * the chain that takes what it hears down to the audio rate.
 *
 * The sources. The engine's sound is the gas leaving and entering its pipes'
 * open ends: the tailpipes, and the intake's mouth (stage 12). An open pipe
 * end small against the wavelengths it radiates (a 50 mm pipe against
 * 0.3-3 m) radiates like a pulsating sphere: a monopole, whose strength is
 * the volume flow Q out of it, the gas the outside air has to make room for.
 * In the open, at a distance r (Kinsler et al., chapter 7),
 *
 *     p(r, t) = rho0 / (4 pi r) dQ/dt(t - r / c),     Q = mdot / rho
 *
 * rho0 and c the outside air's density and speed of sound, mdot the flow out
 * of the pipe and rho the density of the gas at its end, whichever way it
 * flows (engine.h: tail_flow).
 *
 * The listener (stage 12) stands somewhere around the car: behind it, beside
 * it, over its bonnet. Each source is a place on the car; the listener hears
 * it r / c later, at 1 / r, and once more off the ground, as if from its
 * image below the ground, RB_GROUND as strong (asphalt reflects most of it):
 *
 *     p(t) = rho0 / (4 pi) d/dt sum_j [ Q_j(t - r_j / c) / r_j + G Q_j(t - r'_j / c) / r'_j ]
 *
 * Only the differences between the paths' delays matter, so the nearest
 * path's is 0. The sources' flows change once a pipe step (48 kHz), so that's
 * when they're kept; a delay between two pipe steps is read between them.
 * Where the paths' lengths differ the sources no longer cancel: a twin
 * exhaust's two tailpipes, heard from beside one, let through what each bank
 * makes on its own (stage 11's cross-plane V8).
 *
 * Stage 12b adds what the 1D gas can't make (noise.h): the flow's noise at
 * each tailpipe and at the intake's mouth, and the engine's own sound from
 * the engine bay. They come as pressures 1 m from their sources, every
 * physics step, so the listener keeps them at that rate and hears each at
 * (1 m) / r, as late as its path is long, straight and off the ground; the
 * engine bay muffled by the car's body unless the bonnet is open
 * (RB_HEAR_FRONT). Only rb_listener_place_noise puts them around the car.
 *
 * The cabin (RB_HEAR_CABIN, stage 12b). The driver hears everything outside
 * through the body, whose transmission loss follows the mass law, 6 dB more
 * every octave (Fahy, Sound and Structural Vibration, chapter 6):
 * RB_CABIN_PASS of the pressure outside below a corner, a one-pole low-pass
 * above it. The firewall lets through more than the floor and the boot, so
 * what comes from in front of the cabin has a higher corner. The engine's
 * torque reaches the cabin through the mounts too (noise.h), and the
 * cabin's lowest mode booms: a peak RB_CABIN_BOOM_DB high around
 * RB_CABIN_BOOM_HZ.
 *
 * The chain, at each audio frame:
 *
 *   what the listener hears, every physics step (RB_FULL_SCALE_PA is an
 *   output of 1)
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

#define RB_FULL_SCALE_PA  40.0   /* Pa at the listener for an output of 1:
                                    126 dB SPL. Heard from behind the car, the
                                    loudest preset (the cross-plane V8, wide
                                    open at 6000 rpm) peaks at 33 Pa            */
#define RB_DC_CUTOFF      10.0   /* Hz: the DC blocker's corner                 */
#define RB_LIMIT_KNEE     0.8    /* the soft limiter leaves samples below this  */

/* ---- the listener ---- */

#define RB_LISTEN_SOURCES 3      /* tailpipe 0, tailpipe 1 (twin exhausts) and
                                    the intake's mouth                          */
#define RB_LISTEN_HISTORY 2048   /* pipe steps of each source's flow kept: 42 ms
                                    at 48 kHz, 14 m of path difference         */
#define RB_GROUND         0.9    /* the ground's reflection, of the pressure    */

/* Where the listener stands (rb_listener_place). A sim starts behind the
 * car: a metre from its tailpipe the cross-plane V8 at full power peaks at
 * 95 Pa, more than twice full scale. */
enum {
    RB_HEAR_TAILPIPE = 0,   /* a microphone 1 m behind tailpipe 0 (stage 11's)   */
    RB_HEAR_BEHIND,         /* someone standing 3 m behind the car (the default) */
    RB_HEAR_SIDE,           /* someone standing 4 m to the side of its middle    */
    RB_HEAR_FRONT,          /* leaning over the open bonnet                       */
    RB_HEAR_CABIN,          /* in the driver's seat (stage 12b)                   */
    RB_HEAR_COUNT
};

/* Stage 12b's sources, heard as pressures 1 m away (noise.h: RB_NOISE_TAIL0,
 * _TAIL1, _MOUTH and _BAY, in that order). */
#define RB_LISTEN_NOISES  4
#define RB_LISTEN_RING    8192   /* physics steps of each kept: 43 ms at 192 kHz */

#define RB_CABIN_PASS     0.3    /* of the pressure outside, below the corners  */
#define RB_CABIN_REAR_HZ  100.0  /* the corner behind (floor, boot) ...         */
#define RB_CABIN_FRONT_HZ 200.0  /* ... and in front (the firewall)            */
#define RB_CABIN_BOOM_HZ  55.0   /* the cabin's lowest mode ...                 */
#define RB_CABIN_BOOM_Q   2.5
#define RB_CABIN_BOOM_DB  8.0    /* ... and how much it lifts the sound there   */

/* The cabin's filters (RB_HEAR_CABIN). */
typedef struct rb_cabin {
    double dt;                         /* the physics step they're designed for */
    double rear_a, front_a;            /* the body's low-passes ...             */
    double rear, front;                /* ... and their states                  */
    double b0, b1, b2, a1, a2;         /* the boom: a peaking filter ...        */
    double x1, x2, y1, y2;             /* ... and its state                     */
} rb_cabin;

typedef struct rb_listener {
    double gain[RB_LISTEN_SOURCES][2];    /* 1 / r of each source's paths (direct,
                                             off the ground), 1/m; 0: no source  */
    double delay[RB_LISTEN_SOURCES][2];   /* pipe steps, the nearest path's 0    */
    double flow[RB_LISTEN_SOURCES][RB_LISTEN_HISTORY];   /* each source's volume
                                             flow out (m^3/s), a ring           */
    int    pos;                           /* the newest pipe step's place in it  */
    double sum;                           /* what arrives now, m^2/s             */
    double heard;                         /* ... as it was at the last physics step */
    double sum_front, heard_front;        /* the same of the sources in front of
                                             the cabin (the mouth)               */

    /* stage 12b: the sources heard as pressures */
    double ngain[RB_LISTEN_NOISES][2];    /* (1 m) / r of each one's paths; 0: none */
    double ndelay[RB_LISTEN_NOISES][2];   /* physics steps, the same reference
                                             as `delay`                          */
    float  ring[RB_LISTEN_NOISES][RB_LISTEN_RING];   /* their pressures (Pa at
                                             1 m), each physics step             */
    int    npos;                          /* the newest step's place in it       */
    double mount;                         /* Pa: the mounts', in the cabin, now  */
    int    inside;                        /* 1: in the cabin (RB_HEAR_CABIN)     */
    rb_cabin cabin;
} rb_listener;

/* The listener at `at` and the sources at pos[0..sources-1] (m; z up from the
 * ground at 0), the ground reflecting `ground` of the pressure (0: none); c
 * (m/s) the speed of sound, pipe_dt (s) how often the sources' flows change.
 * A source can be left out with a NULL position. Clears what was heard. */
void   rb_listener_set(rb_listener *l, const double *const pos[RB_LISTEN_SOURCES],
                       const double at[3], double ground, double c, double pipe_dt);

/* The listener at one of the RB_HEAR_ places around a front-engined car with
 * `tailpipes` tailpipes (1 or 2) and its intake's mouth behind the grille
 * (audio.c has the car's measurements). Returns 0, or -1 for a bad place. */
int    rb_listener_place(rb_listener *l, int where, int tailpipes, double c, double pipe_dt);

/* How far source j's direct path is (m), 0 if there's no such source. */
double rb_listener_distance(const rb_listener *l, int j);

/* Fills every source's history with the flows q (m^3/s), as if they had
 * always been so: nothing jumps (a new engine, a new place). */
void   rb_listener_prime(rb_listener *l, const double q[RB_LISTEN_SOURCES]);

/* One pipe step's flows out of the sources (m^3/s). */
void   rb_listener_push(rb_listener *l, const double q[RB_LISTEN_SOURCES]);

/* Puts stage 12b's sources around the car for a listener rb_listener_place
 * put at `where` (the same place and tailpipes): each tailpipe's and the
 * mouth's noise where they are, and the engine bay, its body muffling it
 * but at RB_HEAR_FRONT. dt (s) is the physics step they come at; if one is
 * nearer than every flow source, the flows' delays grow to match. Returns
 * 0, or -1 for a bad place. */
int    rb_listener_place_noise(rb_listener *l, int where, int tailpipes, double c,
                               double pipe_dt, double dt);

/* One physics step's pressures (Pa at 1 m) from stage 12b's sources, and the
 * mounts' in the cabin (Pa). Call it every physics step before hearing. */
void   rb_listener_push_noise(rb_listener *l, const double p[RB_LISTEN_NOISES], double mount);

/* The pressure (Pa) heard over a physics step of dt (s), the outside air's
 * density rho0: rho0 / (4 pi) d(sum)/dt. Between pipe steps the sum doesn't
 * change, so a pipe step's change comes out at the physics step after its
 * push; the decimator's filter smooths that back to the derivative. Then
 * stage 12b's sources, and in the cabin everything through its filters. */
double rb_listener_hear(rb_listener *l, double rho0, double dt);

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
