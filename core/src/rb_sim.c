/*
 * rb_sim.c - lifecycle, the step/render loop and telemetry.
 *
 * Stage 1: the model is a placeholder sine oscillator. It exists to prove
 * the plumbing (parameters -> physics substeps -> reduction to the audio
 * rate -> output stage) before any engine physics is written. Later
 * stages replace model_step() with the engine.
 */
#include "rb_internal.h"

#include <math.h>
#include <string.h>

const char *rb_version(void)
{
    return RB_VERSION_STRING;
}

void rb_config_default(rb_config *cfg)
{
    if (!cfg) return;
    cfg->sample_rate = 48000.0;
    cfg->oversample  = 4;
}

size_t rb_sizeof(void)
{
    return sizeof(rb_sim);
}

static int config_is_valid(const rb_config *c)
{
    return isfinite(c->sample_rate)
        && c->sample_rate >= RB_MIN_SAMPLE_RATE
        && c->sample_rate <= RB_MAX_SAMPLE_RATE
        && c->oversample >= 1
        && c->oversample <= RB_MAX_OVERSAMPLE;
}

double rb_onepole_coef(double dt, double tau)
{
    /* Discretised first-order low-pass:  y += a * (target - y).
     * With a = 1 - exp(-dt / tau) a step input reaches 1 - 1/e (63%)
     * after tau seconds, whatever dt is. */
    if (!(tau > 0.0)) return 1.0;
    return 1.0 - exp(-dt / tau);
}

int rb_init(rb_sim *sim, const rb_config *cfg)
{
    rb_config c;

    if (!sim) return RB_ERR_ARG;
    if (cfg) c = *cfg;
    else     rb_config_default(&c);
    if (!config_is_valid(&c)) return RB_ERR_CONFIG;

    memset(sim, 0, sizeof *sim);
    sim->cfg = c;
    sim->dt  = 1.0 / (c.sample_rate * (double)c.oversample);
    sim->smooth_a_step  = rb_onepole_coef(sim->dt, RB_SMOOTH_TAU_S);
    sim->smooth_a_frame = rb_onepole_coef(1.0 / c.sample_rate, RB_SMOOTH_TAU_S);

    for (int i = 0; i < RB_PARAM_COUNT; i++)
        sim->param[i] = rb_param_default(i);

    rb_reset(sim);
    return RB_OK;
}

void rb_reset(rb_sim *sim)
{
    if (!sim) return;

    sim->steps = 0;

    /* The output gain starts where it is set. The model starts silent and
     * fades in through its own smoothed level, so (re)starting never
     * clicks. */
    sim->gain_s    = sim->param[RB_PARAM_MASTER_GAIN];
    sim->out_peak  = 0.0;
    sim->out_rms   = 0.0;
    sim->nonfinite = 0;

    sim->tone_phase   = 0.0;
    sim->tone_level_s = 0.0;
    sim->mic          = 0.0;
}

/* One physics substep of the model. Stage 1: a phase-continuous sine. */
static inline void model_step(rb_sim *sim)
{
    sim->tone_level_s += sim->smooth_a_step
                       * (sim->param[RB_PARAM_TEST_TONE_LEVEL] - sim->tone_level_s);

    /* Accumulate phase instead of computing sin(2*pi*f*t): when f changes
     * the wave carries on from where it was, so there is no jump. */
    sim->tone_phase += RB_TWO_PI * sim->param[RB_PARAM_TEST_TONE_HZ] * sim->dt;
    if (sim->tone_phase >= RB_TWO_PI)
        sim->tone_phase = fmod(sim->tone_phase, RB_TWO_PI);

    sim->mic = sim->tone_level_s * sin(sim->tone_phase);
}

void rb_step(rb_sim *sim)
{
    model_step(sim);
    sim->steps++;
}

int rb_render_block(rb_sim *sim, float *out, int frames)
{
    if (!sim || !out || frames < 0) return RB_ERR_ARG;

    const int    os     = sim->cfg.oversample;
    const double inv_os = 1.0 / (double)os;
    double peak  = 0.0;
    double sumsq = 0.0;

    for (int i = 0; i < frames; i++) {
        /* 1. Run the physics `os` substeps for this audio sample. */
        double acc = 0.0;
        for (int k = 0; k < os; k++) {
            model_step(sim);
            acc += sim->mic;
        }
        sim->steps += (uint64_t)os;

        /* 2. Reduce to the audio rate. Averaging is fine for a test tone;
         *    stage 11 replaces it with a proper anti-alias filter. */
        double y = acc * inv_os;

        /* 3. Output stage: smoothed gain, NaN/inf guard, hard clamp. */
        sim->gain_s += sim->smooth_a_frame
                     * (sim->param[RB_PARAM_MASTER_GAIN] - sim->gain_s);
        y *= sim->gain_s;
        if (!isfinite(y)) {
            y = 0.0;
            sim->nonfinite++;
        }
        if (y > 1.0)       y = 1.0;
        else if (y < -1.0) y = -1.0;

        out[i] = (float)y;
        if (fabs(y) > peak) peak = fabs(y);
        sumsq += y * y;
    }

    sim->out_peak = peak;
    sim->out_rms  = frames > 0 ? sqrt(sumsq / (double)frames) : 0.0;
    return frames;
}

double rb_get_telemetry(const rb_sim *sim, int id)
{
    if (!sim) return NAN;
    switch (id) {
    case RB_TLM_TIME_S:    return (double)sim->steps * sim->dt;
    case RB_TLM_STEPS:     return (double)sim->steps;
    case RB_TLM_OUT_PEAK:  return sim->out_peak;
    case RB_TLM_OUT_RMS:   return sim->out_rms;
    case RB_TLM_NONFINITE: return (double)sim->nonfinite;
    default:               return NAN;
    }
}

double rb_sample_rate(const rb_sim *sim) { return sim ? sim->cfg.sample_rate : NAN; }
int    rb_oversample(const rb_sim *sim)  { return sim ? sim->cfg.oversample : 0; }
double rb_physics_dt(const rb_sim *sim)  { return sim ? sim->dt : NAN; }
