/* Read-only diagnostic for the 2026-10-22 sound audit.
 * These observations expose current behavior; they are not acceptance tests
 * declaring the behavior physically correct. No audio files are produced.
 * Link with the native core objects, as documented in the audit report. */
#include "engine.h"
#include "noise.h"
#include "presets.h"
#include "rb.h"
#include "scene.h"
#include "valve.h"
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
#define RATE 192000.0
static rb_engine engine;
static rb_noise noise;
static rb_scene scene;

static double output_rms(int output)
{
    double sum = 0.0;
    for (int k = 0; k < 40000; k++) {
        rb_noise_step(&noise, &engine, RB_NOISE_FLOW);
        if (k >= 10000) sum += noise.out[output] * noise.out[output];
    }
    return sqrt(sum / 30000.0);
}

int main(void)
{
    rb_engine_desc d;
    puts("NOISE_PATH preset legacy_delay_ms full_straight_length_delay_ms power_pass systems");
    for (int p = 0; p < rb_preset_count(); p++) {
        if (rb_preset_desc(p, &d) || rb_engine_init(&engine, &d, 1.0/RATE)) return 1;
        rb_noise_init(&noise, &engine, 3);
        const double full = d.header_length[0] + d.exhaust_pipe_length + d.center_length
                          + d.link_length + d.muffler_length + d.tailpipe_length;
        printf("%s %.9g %.9g %.9g %d\n", rb_preset_key(p),
               1000.0 * noise.blow_delay[0]/RATE, 1000.0 * full/580.0,
               noise.blow_pass, engine.systems);
    }

    /* Keep upstream flow fixed to isolate the synthesis/routing from gas
     * solver responses. A real shared junction cannot isolate one branch's
     * acoustic pressure at every frequency. */
    puts("ONE_VALVE preset tail0_rms_pa tail1_rms_pa");
    const int split[] = {2, 4};
    for (int p = 0; p < 2; p++) {
        rb_preset_desc(split[p], &d);
        rb_engine_init(&engine, &d, 1.0/RATE);
        engine.exhaust[0].area = 2e-4;
        engine.exhaust[0].mdot = 0.01;
        rb_noise_init(&noise, &engine, 3);
        const double first = output_rms(RB_NOISE_TAIL0);
        rb_noise_init(&noise, &engine, 3);
        const double second = output_rms(RB_NOISE_TAIL1);
        printf("%s %.12g %.12g\n", rb_preset_key(split[p]), first, second);
    }

    rb_preset_desc(1, &d);
    puts("CATALYST laminar power_pass");
    for (int k = 0; k < 2; k++) {
        d.catalyst_laminar = k * 25.0;
        rb_engine_init(&engine, &d, 1.0/RATE);
        rb_noise_init(&noise, &engine, 3);
        printf("%.9g %.9g\n", d.catalyst_laminar, noise.blow_pass);
    }

    /* Independently evaluate sonic throat conditions, rather than forcing
     * impossible mass flows above the nozzle law's physical maximum. */
    rb_preset_desc(0, &d);
    rb_engine_init(&engine, &d, 1.0/RATE);
    rb_orifice *jet = &engine.intake[0];
    const double air[RB_NSPECIES] = {1.0, 0.0, 0.0};
    rb_chamber_init(jet->a, jet->a->V, 200000.0, 300.0, air);
    rb_chamber_init(jet->b, jet->b->V, 20000.0, 300.0, air);
    jet->area = 2e-4;
    jet->mdot = rb_nozzle_mdot(jet->area*jet->cd, jet->a->p, jet->a->T,
                             jet->a->gas.R, 1.4, jet->b->p);
    rb_noise_init(&noise, &engine, 3);
    rb_noise_step(&noise, &engine, RB_NOISE_FLOW);
    const double sound = sqrt(1.4*jet->a->gas.R*jet->a->T);
    const double inferred = jet->mdot/(jet->a->rho*jet->area*jet->cd*sound);
    const double throat_u2 = sound*sound*2.0/2.4;
    const double sonic_power = 2e-3*0.5*jet->mdot*throat_u2;
    printf("CHOKED_JET code_mach=%.12g throat_mach=1 code_power_w=%.12g "
           "sonic_throat_power_same_eta_w=%.12g difference_db=%.9g\n",
           inferred, noise.intake_power, sonic_power,
           10.0*log10(noise.intake_power/sonic_power));

    rb_preset_desc(0, &d);
    rb_engine_init(&engine, &d, 1.0/RATE);
    engine.rpm = 3000.0;
    engine.ignition = 1;
    engine.variation = 1.0;
    int events = 0, early = 0, early_heat_steps = 0;
    double minimum = 0.0, early_heat = 0.0;
    const double step = 6.0*engine.rpm/RATE;
    const double ivc = rb_valve_closes(&d.intake);
    for (int k = 0; k < 153600; k++) { /* 20 cycles */
        const double previous = rb_engine_cylinder_angle(&engine, 0);
        double ahead = ivc - previous;
        if (ahead < 0.0) ahead += 720.0;
        rb_engine_step(&engine);
        if (ahead < step) {
            events++;
            const double delta = rb_cycle_offset(engine.burn[0].start_angle,
                                                 engine.burn[0].spark_angle);
            if (delta < 0.0) early++;
            if (delta < minimum) minimum = delta;
        }
        const double angle = rb_engine_cylinder_angle(&engine, 0);
        const double delta = rb_cycle_offset(angle, engine.burn[0].spark_angle);
        if (delta < 0.0 && delta > -90.0 && engine.burn[0].heat > 0.0) {
            early_heat_steps++;
            early_heat += engine.burn[0].heat;
        }
    }
    printf("IGNITION cylinder0_ivc_events=%d early_starts=%d most_early_degrees=%.9g "
           "heat_steps_before_commanded_spark=%d heat_before_commanded_spark_j=%.12g\n",
           events, early, minimum, early_heat_steps, early_heat);

    puts("VARIATION_SWITCH variation same_charge_demand_degrees scheduled_duration_degrees");
    for (int k = 0; k < 2; k++) {
        rb_engine_init(&engine, &d, 1.0/RATE);
        engine.rpm = 3000.0;
        engine.ignition = 1;
        engine.variation = k ? 1e-9 : 0.0;
        engine.spark_map = 0;
        engine.spark = 25.0;
        int found = 0;
        for (int j = 0; j < 8000; j++) {
            const double angle = rb_engine_cylinder_angle(&engine, 0);
            double ahead = ivc-angle;
            if (ahead < 0.0) ahead += 720.0;
            if (ahead < step) { found = 1; break; }
            rb_engine_step(&engine);
        }
        if (!found) return 2;
        const double mixture[RB_NSPECIES] = {
            0.4*RB_AFR_STOICH/(RB_AFR_STOICH+1.0), 0.4/(RB_AFR_STOICH+1.0), 0.6};
        rb_chamber_init(&engine.cyl[0].gas, engine.cyl[0].gas.V, 122000.0, 360.0, mixture);
        engine.trim_fuel[0] = 0.0;
        rb_engine_step(&engine);
        printf("%.12g %.12g %.12g\n", engine.variation,
               engine.burn[0].demand_duration, engine.burn[0].duration);
    }

    puts("GRID preset high_cells ultra_cells high_max_dx_m ultra_max_dx_m");
    for (int p = 0; p < rb_preset_count(); p++) {
        int counts[2] = {0, 0};
        double largest[2] = {0, 0};
        rb_preset_desc(p, &d);
        for (int quality = 0; quality < 2; quality++) {
            rb_engine_init_at(&engine, &d, 1.0/(RATE*(quality+1)), 48000.0*(quality+1));
            for (int j = 0; j < rb_engine_pipe_count(&engine); j++) {
                const rb_pipe *pipe = rb_engine_pipe(&engine, j);
                counts[quality] += pipe->cells;
                largest[quality] = fmax(largest[quality], pipe->dx);
            }
        }
        printf("%s %d %d %.9g %.9g\n", rb_preset_key(p), counts[0], counts[1], largest[0], largest[1]);
    }

    rb_scene_init(&scene, RB_SCENE_ASPHALT, 48000.0, 343.0, 2, 0.0611);
    puts("DELAY ear source direct_fraction linear_delay_gain_16khz_db");
    for (int ear = 0; ear < 2; ear++) for (int j = 0; j < 4; j++) {
        const double a = scene.path[ear][j][0].delay;
        const double part = a-floor(a), w = 2.0*PI*16000.0/48000.0;
        const double gain = hypot(1.0-part+part*cos(w), part*sin(w));
        printf("%d %d %.9g %.9g\n", ear, j, part, 20.0*log10(gain));
    }
    return 0;
}
