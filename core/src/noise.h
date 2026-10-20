/*
 * noise.h - the sound the 1D gas can't make (stage 12b): the flow's noise,
 * and the engine's own, mechanical sound.
 *
 * The pipes (stage 10) carry the engine's pulses as plane waves up to about
 * c / (pi dx): 2.4 kHz in the exhaust's 7.5 cm cells. Real engines keep a
 * broadband floor far above that (RESEARCH.md, "Sound realism"), from two
 * places the 1D gas doesn't reach.
 *
 * The flow. Where gas moves fast, its turbulence radiates noise:
 *   - a jet into the open air, the tailpipe's. Lighthill's law gives its
 *     acoustic power, W = K rho0 U^8 D^2 / c0^5 (U the jet's speed, D its
 *     diameter, K about 4e-5 for a cold jet; Lighthill 1952, Goldstein 1976).
 *     A hot jet this slow is louder, by about T_jet / T0 in power (Tanna
 *     1977). At 1 m: p_rms = sqrt(rho0 c0 W / (4 pi)). Its spectrum peaks
 *     near the Strouhal number f D / U = 0.2 and falls by 6 dB an octave of
 *     spectral density above that.
 *   - jets inside the engine: the exhaust valves' at blowdown (the gas at
 *     several bar, choked through the valve's gap) and the throttle's at part
 *     load. A share eta = ETA M^5 of the jet's stream power, 1/2 mdot U^2,
 *     becomes sound (M = U / c, at most 1: a choked valve's noise as in IEC
 *     60534-8-3, simplified to one law). It travels with the gas to the open
 *     end: the exhaust valves' down the exhaust (the catalyst and the muffler
 *     taking their share) to the tailpipe, the throttle's back through the
 *     air box and its filter to the intake's mouth. Such small, fast jets put
 *     their sound high (their Strouhal peaks are above the audible band);
 *     what comes out starts near 1 kHz, where the pipes' own sound fades,
 *     and falls 6 dB an octave of spectral density, 12 above where the way
 *     out takes off the highs.
 * Each is white noise whose level follows the gas every physics step, so it
 * comes in bursts with each pulse (the rasp) and goes quiet on the overrun.
 *
 * The engine. The cylinders' pressure and the parts' impacts shake the block
 * and the head, which ring at frequencies of their own, and the engine bay
 * radiates that: modal synthesis, a few damped resonators (modes) struck by
 *   - the pistons' force, p A, at each step's change of it: combustion
 *     noise. The block lets high frequencies through far better than low
 *     ones (its attenuation curve), so the force's quick rises count;
 *   - piston slap: at each dead centre the side thrust changes sign, and the
 *     piston crosses its clearance to hit the other side of the bore, as hard
 *     as the axial force on it then (the gas's less the parts' inertia).
 *     Loudest after firing TDC, so it follows the load;
 *   - the valves seating and lifting off: as fast as the cam's ramps move
 *     them, which follows the rpm: the ticking;
 *   - the injectors' needles, a click each injection.
 * The modes' frequencies come down for a bigger bore, as a bigger block's do.
 *
 * Everything comes out as pressure 1 m from its source (Pa), each physics
 * step; the listener (audio.h) places the sources and hears them. A seeded
 * generator makes the noise, so a render or a test always comes out the same.
 */
#ifndef RB_NOISE_H
#define RB_NOISE_H

#include "engine.h"

#include <stdint.h>

/* The sources, as the listener places them (audio.h): each tailpipe's mouth,
 * the intake's mouth and the engine bay. RB_NOISE_MOUNT isn't placed: it's
 * what the engine's mounts pass into the body, heard only in the cabin. */
enum {
    RB_NOISE_TAIL0 = 0, RB_NOISE_TAIL1, RB_NOISE_MOUTH, RB_NOISE_BAY,
    RB_NOISE_PLACED,
    RB_NOISE_MOUNT = RB_NOISE_PLACED,
    RB_NOISE_OUTPUTS
};

/* What rb_noise_step makes. */
enum { RB_NOISE_FLOW = 1, RB_NOISE_ENGINE = 2 };

/* The engine's impacts, by kind (rb_noise's bookkeeping). */
enum { RB_IMPACT_SLAP = 0, RB_IMPACT_SEAT, RB_IMPACT_LIFT, RB_IMPACT_INJECTOR, RB_IMPACT_KINDS };

#define RB_NOISE_MODES 6        /* the block's and head's resonances          */
#define RB_NOISE_DELAY 4096     /* physics steps the exhaust valves' noise can
                                   take down the pipes: 21 ms at 192 kHz      */

/* A band of noise: white noise through a one-pole high-pass and two one-pole
 * low-passes (its spectral density falls 6 dB an octave above the first,
 * 12 above the second), scaled so its RMS is 1 (`norm`). */
typedef struct rb_noise_band {
    double hp, lp, top;      /* the filters' coefficients                     */
    double norm;             /* 1 / the RMS white noise of RMS 1 comes out with */
    double hp_lp, mid, out;  /* their states: the high-pass's own low-pass,
                                after the first low-pass, the output          */
} rb_noise_band;

/* One mode: a two-pole resonator, y[n] = x[n] + a1 y[n-1] - a2 y[n-2], fed
 * x = sin(w dt) times the impulses that strike it, so an impulse of height A
 * rings at amplitude A (decaying), whatever the physics rate. */
typedef struct rb_noise_mode {
    double a1, a2, in;  /* coefficients, and the input's scale sin(w dt)      */
    double y1, y2;      /* state                                             */
} rb_noise_mode;

typedef struct rb_noise {
    uint64_t state;     /* the noise generator (xorshift64*)                    */
    double   dt;        /* s, the physics step                                  */

    /* the flow */
    double   tail_area[RB_MAX_COLLECTORS];   /* m^2, each tailpipe's           */
    double   tail_d;                         /* m, their diameter              */
    double   jet_hp, jet_top;                /* the jets' fixed filters        */
    double   jet_hp_lp[RB_MAX_COLLECTORS], jet_mid[RB_MAX_COLLECTORS],
             jet_out[RB_MAX_COLLECTORS];     /* ... their states               */
    rb_noise_band blow[RB_MAX_COLLECTORS];   /* the exhaust valves' noise, at
                                              * each tailpipe                   */
    double   blow_pass;                      /* of its power, through the
                                              * catalyst and the muffler        */
    float    blow_ring[RB_MAX_COLLECTORS][RB_NOISE_DELAY];   /* on its way down */
    int      blow_delay[RB_MAX_CYLINDERS];   /* physics steps, each cylinder's  */
    int      blow_system[RB_MAX_CYLINDERS];  /* the tailpipe it comes out of    */
    int      blow_pos;
    rb_noise_band hiss;                      /* the throttle's, at the mouth    */

    /* the engine */
    rb_noise_mode mode[RB_NOISE_MODES];
    double   force;                          /* N: the pistons' force, last step */
    double   angle[RB_MAX_CYLINDERS];        /* each cylinder's cycle angle then */
    double   recip_mass;                     /* kg: a piston, pin and rod's top  */
    double   mount_hp_a, mount_lp_a;         /* the mounts' filters ...          */
    double   mount_hp, mount_lp;             /* ... and their states             */

    /* this physics step: Pa at 1 m from each placed source, and the mounts' */
    double   out[RB_NOISE_OUTPUTS];

    /* bookkeeping, for tests and tools: the impacts since rb_noise_init by
     * kind (RB_IMPACT_*), and the sum of their sizes squared (Pa^2 at 1 m,
     * before IMPACT_SPREAD) */
    long     impacts[RB_IMPACT_KINDS];
    double   impact_energy[RB_IMPACT_KINDS];
} rb_noise;

/* Ready for the engine as it is now (its pipes, its bore, its physics step),
 * the generator seeded with `seed`: the same seed, the same noise. */
void rb_noise_init(rb_noise *n, const rb_engine *e, uint64_t seed);

/* One physics step, after rb_engine_step: the flow's noise and/or the
 * engine's (RB_NOISE_FLOW | RB_NOISE_ENGINE) into n->out; what isn't asked
 * for is 0. */
void rb_noise_step(rb_noise *n, const rb_engine *e, int what);

#endif /* RB_NOISE_H */
