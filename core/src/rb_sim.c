/*
 * rb_sim.c - lifecycle, the step/render loop and telemetry.
 *
 * The plumbing: parameters -> physics substeps -> reduction to the audio rate
 * -> output stage. Stage 1's model is a placeholder sine oscillator, which
 * the browser plays until stage 14. Stage 11 adds the engine as a source (the
 * sim holds it since stage 8, rb_engine_api.c): a listener hears its pipe
 * ends (stage 12: from a place around the car), and the reduction to the
 * audio rate is an anti-alias filter (audio.h).
 */
#include "rb_internal.h"

#include "presets.h"

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
    cfg->pipe_rate   = RB_PIPE_RATE;
}

int rb_config_quality(rb_config *cfg, int quality)
{
    if (!cfg || quality < 0 || quality >= RB_QUALITY_COUNT
        || !(cfg->sample_rate >= RB_MIN_SAMPLE_RATE && cfg->sample_rate <= RB_MAX_SAMPLE_RATE))
        return RB_ERR_ARG;
    const double physics = (quality == RB_QUALITY_LOW) ? 96000.0 : 192000.0;
    long os = lround(physics / cfg->sample_rate);
    if (os < 1) os = 1;
    if (os > RB_MAX_OVERSAMPLE) os = RB_MAX_OVERSAMPLE;
    cfg->oversample = (int)os;
    cfg->pipe_rate = (quality == RB_QUALITY_HIGH) ? RB_PIPE_RATE : 0.5 * RB_PIPE_RATE;
    return RB_OK;
}

size_t rb_sizeof(void)
{
    return sizeof(rb_sim);
}

/* A valid config (pipe_rate 0 has already become the default). */
static int config_is_valid(const rb_config *c)
{
    return isfinite(c->sample_rate)
        && c->sample_rate >= RB_MIN_SAMPLE_RATE
        && c->sample_rate <= RB_MAX_SAMPLE_RATE
        && c->oversample >= 1
        && c->oversample <= RB_MAX_OVERSAMPLE
        && c->pipe_rate >= RB_PIPE_RATE_MIN
        && c->pipe_rate <= RB_PIPE_RATE_MAX;
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
    if (c.pipe_rate == 0.0) c.pipe_rate = RB_PIPE_RATE;
    if (!config_is_valid(&c)) return RB_ERR_CONFIG;

    memset(sim, 0, sizeof *sim);
    sim->cfg = c;
    sim->dt  = 1.0 / (c.sample_rate * (double)c.oversample);
    sim->smooth_a_step  = rb_onepole_coef(sim->dt, RB_SMOOTH_TAU_S);
    sim->smooth_a_frame = rb_onepole_coef(1.0 / c.sample_rate, RB_SMOOTH_TAU_S);

    /* The sound's chain (the oversampling factor is within the decimator's
     * range: RB_MAX_OVERSAMPLE). */
    _Static_assert(RB_MAX_OVERSAMPLE <= RB_DECIMATOR_MAX_FACTOR,
                   "the decimator must take every oversampling factor");
    sim->source = RB_SOURCE_TONE;
    sim->hear = RB_HEAR_BEHIND;
    sim->layers = RB_LAYER_ALL;
    rb_decimator_init(&sim->aa, c.oversample);
    rb_dc_blocker_init(&sim->dc, RB_DC_CUTOFF, c.sample_rate);

    for (int i = 0; i < RB_PARAM_COUNT; i++)
        sim->param[i] = rb_param_default(i);

    sim->preset = 0;
    rb_preset_desc(0, &sim->engine.desc);   /* rb_reset builds the engine from it */

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
    sim->limited   = 0;
    rb_decimator_reset(&sim->aa);
    rb_dc_blocker_reset(&sim->dc);

    sim->tone_phase   = 0.0;
    sim->tone_level_s = 0.0;
    sim->mic          = 0.0;

    /* The engine starts again at rest, as it's described. The description
     * is always a possible engine, so this can't fail. */
    const rb_engine_desc d = sim->engine.desc;
    rb_engine_init_at(&sim->engine, &d, sim->dt, sim->cfg.pipe_rate);
    rb_sim_listen(sim);
}

_Static_assert(RB_LISTEN_NOISES == RB_NOISE_PLACED, "the listener hears every placed noise source");

void rb_sim_listen(rb_sim *sim)
{
    const rb_engine *e = &sim->engine;
    const rb_chamber *air = &e->exhaust_air;
    const double cv = rb_gas_cv(&air->gas, air->T);
    const double c = sqrt((cv + air->gas.R) / cv * air->gas.R * air->T);
    const int pipes = (sim->layers & RB_LAYER_PIPES) != 0;
    const double q[RB_LISTEN_SOURCES] = { pipes * e->tail_flow[0], pipes * e->tail_flow[1], pipes * e->mouth_flow };
    rb_listener_place(&sim->listener, sim->hear, e->systems, c, e->pipe_dt);
    rb_listener_place_noise(&sim->listener, sim->hear, e->systems, c, e->pipe_dt, sim->dt);
    rb_listener_prime(&sim->listener, q);
    sim->pipe_steps_heard = e->pipe_steps;
    rb_noise_init(&sim->noise, e, RB_NOISE_SEED);
}

/* One physics substep of the model: the engine and what the listener hears
 * of it, or stage 1's phase-continuous sine. */
static inline void model_step(rb_sim *sim)
{
    if (sim->source == RB_SOURCE_ENGINE) {
        rb_engine *e = &sim->engine;
        rb_engine_step(e);
        if (e->pipe_steps != sim->pipe_steps_heard) {   /* the pipe ends' new flows */
            const int pipes = (sim->layers & RB_LAYER_PIPES) != 0;
            const double q[RB_LISTEN_SOURCES] = { pipes * e->tail_flow[0], pipes * e->tail_flow[1],
                                                  pipes * e->mouth_flow };
            rb_listener_push(&sim->listener, q);
            sim->pipe_steps_heard = e->pipe_steps;
        }
        /* Stage 12b: the flow's noise and the engine's own sound. */
        const int what = ((sim->layers & RB_LAYER_FLOW) ? RB_NOISE_FLOW : 0)
                       | ((sim->layers & RB_LAYER_ENGINE) ? RB_NOISE_ENGINE : 0);
        rb_noise_step(&sim->noise, e, what);
        rb_listener_push_noise(&sim->listener, sim->noise.out, sim->noise.out[RB_NOISE_MOUNT]);
        sim->mic = rb_listener_hear(&sim->listener, e->exhaust_air.rho, sim->dt) / RB_FULL_SCALE_PA;
        return;
    }

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

    const int os = sim->cfg.oversample;
    double peak  = 0.0;
    double sumsq = 0.0;

    for (int i = 0; i < frames; i++) {
        /* 1. Run the physics `os` substeps for this audio sample, feeding
         *    what the listener hears to the anti-alias filter. A NaN or inf
         *    becomes silence (and is counted) before it reaches the filters'
         *    memories, and RB_MIC_MAX keeps their sums finite. */
        for (int k = 0; k < os; k++) {
            model_step(sim);
            double x = sim->mic;
            if (!isfinite(x)) {
                x = 0.0;
                sim->nonfinite++;
            } else if (fabs(x) > RB_MIC_MAX) {
                x = (x > 0.0) ? RB_MIC_MAX : -RB_MIC_MAX;
            }
            rb_decimator_push(&sim->aa, x);
        }
        sim->steps += (uint64_t)os;

        /* 2. Reduce to the audio rate: the anti-alias filter's output, every
         *    os-th substep. */
        double y = rb_decimator_out(&sim->aa);

        /* 3. Output stage: DC blocker, smoothed gain, soft limiter (never
         *    past full scale), and a last guard. */
        y = rb_dc_blocker_step(&sim->dc, y);
        sim->gain_s += sim->smooth_a_frame
                     * (sim->param[RB_PARAM_MASTER_GAIN] - sim->gain_s);
        y *= sim->gain_s;
        if (fabs(y) > RB_LIMIT_KNEE) sim->limited++;
        y = rb_soft_limit(y);
        if (!isfinite(y)) {
            y = 0.0;
            sim->nonfinite++;
        }

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
    case RB_TLM_LIMITED:   return (double)sim->limited;
    default:               return NAN;
    }
}

double rb_sample_rate(const rb_sim *sim) { return sim ? sim->cfg.sample_rate : NAN; }
int    rb_oversample(const rb_sim *sim)  { return sim ? sim->cfg.oversample : 0; }
double rb_physics_dt(const rb_sim *sim)  { return sim ? sim->dt : NAN; }
double rb_pipe_rate(const rb_sim *sim)   { return sim ? 1.0 / sim->engine.pipe_dt : NAN; }
