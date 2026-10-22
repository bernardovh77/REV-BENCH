/* Fixed-geometry, fixed-48-kHz-pipe timestep convergence at the new top end.
 * Build against the native core objects; see docs/stage-13b-r.md. */
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include <math.h>
#include <stdio.h>

static rb_engine e;

int main(void)
{
    static const int presets[] = { 1, 2, 3 };
    static const double rates[] = { 192000.0, 384000.0, 768000.0 };
    puts("preset,rate_hz,rpm,cycles,ve,pmep_pa,brake_torque_nm,mass_error_kg,energy_error_j,repairs");
    for (int p = 0; p < 3; p++) for (int r = 0; r < 3; r++) {
        rb_engine_desc d;
        if (rb_preset_desc(presets[p], &d) || rb_engine_init_at(&e, &d, 1.0/rates[r], 48000.0)) return 1;
        e.ignition = 1;
        e.throttle_pos = 1.0;
        e.rpm = d.rev_limit-250.0;
        double m0, u0, m1, u1;
        rb_engine_contents(&e, &m0, &u0, NULL);
        const long steps = lround(100.0*120.0/e.rpm*rates[r]);
        int n = 0;
        double ve = 0.0, pmep = 0.0, torque = 0.0;
        for (long k = 0; k < steps; k++) {
            const long cycle = e.cycles;
            rb_engine_step(&e);
            if (e.cycles != cycle && e.cycles > 50) {
                n++; ve += e.result.ve; pmep += e.result.pmep; torque += e.result.brake_torque;
            }
        }
        if (!n) return 2;
        rb_engine_contents(&e, &m1, &u1, NULL);
        int repairs = e.plenum.repairs+e.airbox.repairs+e.junction.repairs;
        for (int i = 0; i < d.cylinders; i++) repairs += e.cyl[i].gas.repairs;
        for (int i = 0; i < e.collectors; i++) repairs += e.collector[i].repairs;
        for (int i = 0; i < rb_engine_pipe_count(&e); i++) {
            const rb_pipe *pipe = rb_engine_pipe(&e, i);
            for (int j = 0; j < pipe->cells; j++) repairs += pipe->cell[j].repairs;
        }
        const double expected = e.energy_in-e.energy_out+e.work_on_gas+e.wall_heat
                              +rb_engine_pipe_heat(&e)+e.heat_released+e.fuel_energy_in;
        printf("%s,%.0f,%.0f,%d,%.12g,%.12g,%.12g,%.12g,%.12g,%d\n",
            rb_preset_key(presets[p]), rates[r], e.rpm, n, ve/n, pmep/n, torque/n,
            m1-m0-(e.mass_in-e.mass_out+e.fuel_in), u1-u0-expected, repairs);
        fflush(stdout);
        if (!isfinite(torque) || repairs) return 3;
    }
    return 0;
}
