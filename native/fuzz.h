/*
 * fuzz.h - random engines, run on their own crank and checked (stage 13).
 *
 * Each run takes a random preset, changes a few of its fields to random
 * values anywhere in their ranges (rb_set_engine, as the browser's controls
 * will), starts it with the key and drives it: the throttle jumping between
 * shut, wide open and anywhere between, loads on the crank, the ignition cut
 * now and then. The sound goes through the whole chain to the audio rate,
 * heard from any of the listener's places (stage 12b: the cabin too).
 * It fails if anything stops being a number (the samples, the speed, any
 * chamber's pressure or temperature) or runs away (the speed beyond
 * RB_FUZZ_MAX_RPM, a pressure beyond RB_FUZZ_MAX_P, a temperature beyond
 * RB_FUZZ_MAX_T). The same seed always makes the same run.
 */
#ifndef RB_FUZZ_H
#define RB_FUZZ_H

#define RB_FUZZ_MAX_RPM 30000.0   /* rpm: past any rev limit the fields allow     */
#define RB_FUZZ_MAX_P   1000e5    /* Pa: ten times a knocking cylinder's peak     */
#define RB_FUZZ_MAX_T   6000.0    /* K: twice the hottest a flame gets            */

/* A field the run changed: which, where and to what, and what rb_set_engine said. */
typedef struct rb_fuzz_change {
    int    field, index, rc;
    double value;
} rb_fuzz_change;

#define RB_FUZZ_MAX_CHANGES 16

typedef struct rb_fuzz_result {
    int    ok;             /* 1: nothing went wrong                               */
    char   what[200];      /* what went wrong, and when                           */
    int    preset;         /* the preset it started from                          */
    int    changes;        /* fields changed (accepted, maybe clamped)            */
    int    tries;          /* fields it tried to change: change[0..tries-1]       */
    rb_fuzz_change change[RB_FUZZ_MAX_CHANGES];
    int    cylinders;      /* the engine it ran                                    */
    int    hear;           /* where the listener stood (RB_HEAR_*, audio.h)       */
    double max_rpm;        /* the extremes it reached                              */
    double max_p;          /* Pa, any chamber                                      */
    double max_T;          /* K, any chamber                                       */
    long   repairs;        /* NaN or negative states the chambers had to repair   */
    double seconds;        /* simulated                                            */
} rb_fuzz_result;

/* One run of `seconds` (s) with physics `oversample` times the 48 kHz audio
 * rate, or at a quality (RB_QUALITY_*; -1: the oversample, pipes at their
 * default rate). Returns 1 if it passed, 0 if not (r->what says why). */
int rb_fuzz_run(unsigned long long seed, double seconds, int oversample, int quality,
                rb_fuzz_result *r);

#endif /* RB_FUZZ_H */
