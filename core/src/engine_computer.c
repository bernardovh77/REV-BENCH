/*
 * engine_computer.c - the engine computer: the spark it chooses, the burn it
 * expects, the rev limiter and the 2-step, the fuel cut on overrun and the idle
 * air valve (see engine.h). Moved from engine.c in stage 130, line for line.
 */
#include "engine_parts.h"

#include "combustion.h"
#include "rand.h"
#include "ignition.h"
#include "fastmath.h"
#include "rb.h"

#include <math.h>

/* The spark advance to use (degrees before firing TDC): the setting, kept
 * within 90 degrees of TDC so the spark always falls between the valves'
 * events (NaN counts as 0). */
double rb_computer_spark_advance(double s)
{
    if (s > 90.0) return 90.0;
    if (s < -90.0) return -90.0;
    return (s == s) ? s : 0.0;
}

/* How the burn's length changes with speed and the charge (stage 9). Two
 * things carry the flame through the cylinder (Heywood, chapter 9; the
 * entrainment-and-burn-up picture of Blizard, Keck and Tabaczynski):
 *   - the turbulent flame front is swept along by the charge's turbulence,
 *     which grows with piston speed, so that part of the burn takes about
 *     the same crank angle at any speed;
 *   - each eddy it swallows then burns up from the inside at the laminar
 *     flame speed S_L, taking about lambda / S_L, where the eddies' Taylor
 *     microscale lambda goes as sqrt(nu L / u'): longer when the charge is
 *     thin (low density, high kinematic viscosity nu) or diluted with burned
 *     gas, which slows S_L. In crank degrees that part goes as
 *     sqrt(rpm nu) / S_L.
 * At the reference (3000 rpm, a wide-open charge) the second part is 40 % of
 * the burn, which makes the burn's crank angle grow as about rpm^0.2 there
 * (with the same charge): slowly, as real engines' burn angles do (Heywood).
 * S_L is gasoline's, slowed by burned gas (Metghalchi and Keck's fit, Turns
 * eq. 8.33); nu = mu / rho with mu ~ T^0.7. The charge is taken at intake
 * valve closing: compressed to the spark, its ratios to the reference stay
 * about the same. Stage041 keeps the charge model at every variation level;
 * the capped policy is an explicit native diagnostic control. EVO truncates
 * a burn that cannot finish. See combustion.c for the fit and range flags.
 * This does not supply a calibrated spark-kernel failure law. */
#define MBT_CA50        8.0       /* deg after TDC: where the computer puts the middle
                                     of the burn (best torque, SAE 852090: 7-8)        */

/* Snapshot the charge model independently of random amplitude. Return the
 * historical capped duration only as the kernel surrogate's scale. */
double rb_computer_burn_duration(const rb_engine *e, rb_burn *b, double rpm)
{
    const rb_charge_burn q = rb_charge_burn_evaluate(b->T_ivc, b->p_ivc, b->residual, b->phi_ivc,
                                                     rpm, e->desc.burn_duration);
    b->combustion_model = e->combustion_model;
    b->flame_flags = q.flags;
    if (b->combustion_model == RB_BURN_CAPPED) b->flame_flags |= RB_FLAME_CAPPED_MODEL;
    b->flame_speed_m_s = q.speed_m_s;
    b->raw_demand_duration = q.raw_demand_deg;
    b->demand_duration = q.demand_deg;
    return q.capped_deg;
}

/* The spark advance for the best torque: the middle of the burn (where the
 * Wiebe curve reaches 1/2) at MBT_CA50 after TDC. */
static double mbt_advance(const rb_engine *e, double duration, double kernel_delay)
{
    const rb_engine_desc *d = &e->desc;
    return kernel_delay + duration * rb_wiebe_quantile(.5, d->wiebe_a, d->wiebe_m) - MBT_CA50;
}

/* At idle the engine computer acts on the speed the engine is heading for:
 * IDLE_LOOKAHEAD on, at the rate it's changing (rpm_smooth - rpm_recent is
 * about how much it changed over the last RPM_RECENT seconds). */
#define RPM_RECENT      0.15      /* s: the speed's smoothing for its rate      */
#define IDLE_LOOKAHEAD  0.5       /* s: how far ahead the idle control looks    */

static double idle_heading(const rb_engine *e)
{
    return e->rpm_smooth + IDLE_LOOKAHEAD / RPM_RECENT * (e->rpm_smooth - e->rpm_recent);
}

/* At idle the computer holds the spark back from its best, a torque reserve,
 * and moves it with the speed the engine is heading for: advanced when it's
 * heading below its idle speed, retarded when above. A spark change acts
 * within a cycle, far sooner than the idle valve's air reaches the cylinders,
 * so the spark steadies the idle while the valve does the slow work (stage 9).
 * It needs to be strong: with the runners (stage 10), an engine idling a few
 * rpm faster pushes less exhaust back into its runners at each overlap (the
 * overlap is over sooner), so it breathes better and makes more torque before
 * the plenum pressure can follow. The six, on the stage 9 spark of 0.1 deg per
 * rpm and with its idle valve held still, swung about 95 rpm every 2.8 s
 * (stage 9's six held steady with its spark held still too). It idles this
 * way with the foot off, running with its crank free, and within
 * IDLE_SPARK_BAND of its idle speed. */
#define IDLE_SPARK      15.0      /* deg before TDC at the idle speed            */
#define IDLE_SPARK_GAIN 0.3       /* deg more per rpm below it                   */
#define IDLE_SPARK_BAND 400.0     /* rpm above the idle speed where idling ends  */

/* The spark advance the engine computer's map gives for a cycle: at idle,
 * on the overrun, or the best torque's. */
static double mapped_spark(const rb_engine *e, double duration, double kernel_delay)
{
    const rb_engine_desc *d = &e->desc;
    const double best = mbt_advance(e, duration, kernel_delay);
    if (!e->hold_speed && d->idle_rpm > 0.0 && e->throttle_pos < 0.01
        && e->rpm_smooth < d->idle_rpm + IDLE_SPARK_BAND) {
        /* ... earlier by the flame's expected development delay, so the
         * burn sits where IDLE_SPARK put it before there was one. */
        const double s = IDLE_SPARK + kernel_delay + IDLE_SPARK_GAIN * (d->idle_rpm - idle_heading(e));
        return (s < 0.0) ? 0.0 : (s < best) ? s : best;
    }
    /* Overrun entry removes torque through ignition before cutting fuel.
     * Scheduling still happens once at IVC, never by reshaping the audio. */
    const double phase = e->overrun_transition > 0.0
                       ? fmin(e->overrun_time / e->overrun_transition, 1.0) : 0.0;
    return best - e->overrun_retard * phase;
}

/* The spark advance the engine computer chooses for a cycle: the cranking
 * spark while the starter turns it, the fixed spark with the map off, else
 * the map's plus the driver's offset (RB_PARAM_SPARK_OFFSET). */
double rb_computer_spark(const rb_engine *e, double duration, double kernel_delay, double rpm)
{
    const rb_engine_desc *d = &e->desc;
    if (!e->hold_speed && rpm < d->cranking_rpm) return d->cranking_spark;
    if (!e->spark_map) return e->spark;
    const double mapped = mapped_spark(e, duration, kernel_delay);
    return e->spark_offset != 0.0 ? mapped + e->spark_offset : mapped;
}

/* Stage 133: 1 while the 2-step holds the engine (armed, and its limit
 * lower than the rev limit): its limit, gap and band stand in for the rev
 * limiter's. */
static int launching(const rb_engine *e)
{
    return e->launch && e->desc.launch_rpm < e->desc.rev_limit;
}

/* The rev limiter's soft cut (stage 12c): within the band below the rev
 * limit (limiter_band, 150 rpm by default; stage 133 made it a field) the
 * engine computer skips single cuts at random, more of them the nearer the
 * limit (a share that grows from none to all across the band), as a racing
 * ECU's soft cut does: held there in gear the engine stutters at the limit,
 * firing on some of its cylinders. Above the limit the hard cut stops every
 * one (engine_computer). Before stage 12c only the hard cut was there: in
 * gear, with a car's inertia on the crank, it took 0.6 s to fall the 100 rpm
 * back to where it fires again; real engines bounce off their limiters 10-30
 * times a second. The 2-step (launching) has its own band. */
static int soft_cut(rb_engine *e)
{
    if (e->hold_speed) return 0;   /* the engine computer is off while held */
    const int l = launching(e);
    const double band = l ? e->desc.launch_band : e->desc.limiter_band;
    if (!(band > 0.0)) return 0;
    const double limit = l ? e->desc.launch_rpm : e->desc.rev_limit;
    const double into = (e->omega * 30.0 / PI - (limit - band)) / band;
    if (!(into > 0.0)) return 0;
    return rb_rand_uniform(&e->rand_cycles) < into;
}

/* What the limiter does to this spark or injection: RB_IGN_HARD_CUT,
 * RB_IGN_SOFT_CUT, or -1 (none). */
int rb_computer_limiter_cut(rb_engine *e)
{
    return e->spark_cut ? RB_IGN_HARD_CUT : (soft_cut(e) ? RB_IGN_SOFT_CUT : -1);
}

/* The engine computer (free crank only, ignition on), every step: the rev
 * limiter, the fuel cut on overrun, and the idle air valve holding the idle
 * speed. They look at the crank speed smoothed over a few firing pulses.
 * Coasting with the foot off the pedal more than 700 rpm above idle, the
 * injectors stop until the engine is back within 400 rpm of idle: no fuel is
 * wasted, and the engine slows down faster. The idle valve opens in
 * proportion to how far below its idle speed the engine is heading
 * (idle_gain_p): the speed half a second on, at the rate it's changing. The
 * air takes about that long to fill the plenum and runners (4 L in the four),
 * so falling towards idle the valve opens before the speed gets there; looking
 * at the speed now, the four dipped 150 rpm below idle after a blip, the
 * plenum still emptied from the fuel cut (stage 10). The rate is the speed's
 * change over the last 0.15 s: over the 0.5 s of the trend, a long coast down
 * from the rev limiter still looked steep when the fuel came back on, and held
 * the valve wide open above idle. All that around a position the valve learns
 * while the engine idles (idle_gain_i): running,
 * foot off the pedal, and its speed steady (the speed smoothed over 0.05 s
 * within 50 rpm of the speed smoothed over 0.5 s). The learning is slow and
 * counts at most 100 rpm of error, so coasting down from high rpm doesn't
 * unlearn the idle position. Below the idle speed it learns even while the
 * speed is still changing: sagging below idle always calls for more air, and
 * waiting for the speed to steady let a load thrown on at idle swing the
 * engine around for seconds before it learned anything (stage 9). */
#define IDLE_VALVE_BASE 0.15      /* opening to start from                     */
#define IDLE_LEARN_MAX  100.0     /* rpm: most error the learning counts       */
#define IDLE_STEADY     50.0      /* rpm: how steady the speed must be to learn */
#define RPM_SMOOTHING   0.05      /* s: the speed's smoothing ...              */
#define RPM_TREND       0.5       /* s: ... and a slower one, for its trend    */
#define FUEL_CUT_ABOVE  700.0     /* rpm above idle: fuel cut on overrun ...   */
#define FUEL_BACK_ABOVE 400.0     /* ... until the engine is back below this   */

void rb_computer_step(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const double rpm = e->omega * 30.0 / PI;
    e->rpm_smooth += (rpm - e->rpm_smooth) * (e->dt / RPM_SMOOTHING);
    e->rpm_trend += (e->rpm_smooth - e->rpm_trend) * (e->dt / RPM_TREND);
    e->rpm_recent += (e->rpm_smooth - e->rpm_recent) * (e->dt / RPM_RECENT);
    if (!e->ignition) {            /* switched off: the valve stays where it is */
        e->spark_cut = e->fuel_cut = 0;
        e->overrun_time = 0.0;
        e->overrun_armed = 0;
        return;
    }

    /* the hard cut (the soft one: soft_cut), at the 2-step's limit while it
     * holds the engine (stage 133) */
    const int l = launching(e);
    const double limit = l ? d->launch_rpm : d->rev_limit;
    const double resume = l ? d->launch_rpm - d->launch_gap : d->rev_limit_resume;
    if (rpm > limit) e->spark_cut = 1;
    else if (rpm < resume) e->spark_cut = 0;

    if (!(d->idle_rpm > 0.0)) {
        e->idle_valve = 0.0;
        e->fuel_cut = 0;
        e->overrun_time = 0.0;
        return;
    }
    const int foot_off = e->throttle_pos < 0.01;   /* NaN counts as on */
    /* This transition belongs to a pedal lift, not the key-start transient.
     * Retain startup's existing overspeed protection until the pedal is used. */
    if (!foot_off) e->overrun_armed = 1;
    else if (e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) e->overrun_armed = 0;
    if (!foot_off || e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) {
        e->fuel_cut = 0;
        e->overrun_time = 0.0;
    } else if (e->rpm_smooth > d->idle_rpm + FUEL_CUT_ABOVE
               || e->overrun_time > 0.0 || e->fuel_cut) {
        const double transition = e->overrun_armed && e->overrun_transition > 0.0
                                && rb_finite(e->overrun_transition)
                                ? e->overrun_transition : 0.0;
        e->overrun_time = fmin(e->overrun_time + e->dt, transition);
        e->fuel_cut = e->overrun_time >= transition;
    }

    const double error = d->idle_rpm - e->rpm_smooth;
    if (e->rpm_smooth > d->cranking_rpm && foot_off
        && (fabs(e->rpm_smooth - e->rpm_trend) < IDLE_STEADY || error > 0.0)) {
        const double counted = (error > IDLE_LEARN_MAX) ? IDLE_LEARN_MAX
                             : (error < -IDLE_LEARN_MAX) ? -IDLE_LEARN_MAX : error;
        e->idle_trim += d->idle_gain_i * counted * e->dt;
        if (e->idle_trim < -IDLE_VALVE_BASE) e->idle_trim = -IDLE_VALVE_BASE;
        if (e->idle_trim > 1.0 - IDLE_VALVE_BASE) e->idle_trim = 1.0 - IDLE_VALVE_BASE;
    }
    const double v = IDLE_VALVE_BASE + e->idle_trim + d->idle_gain_p * (d->idle_rpm - idle_heading(e));
    e->idle_valve = (v > 0.0) ? ((v < 1.0) ? v : 1.0) : 0.0;
}
