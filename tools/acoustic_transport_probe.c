/* Standalone diagnostic: unchanged production pipe kernels in a periodic,
 * equal-bore straight duct. No engine/preset/audio-output tuning is changed.
 * Build and interpretation are recorded beside the generated CSV evidence. */
#define _POSIX_C_SOURCE 200809L
#include "pipe.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PI 3.14159265358979323846
#define PRESSURE 101325.0
#define LENGTH 33.6
#define DURATION 0.0025
static double probe_duration = DURATION;
static const double AIR[RB_NSPECIES] = {[RB_AIR] = 1.0};

typedef struct {
    int count, cells_per, n;
    double dx, rho, sound, u, area;
    rb_pipe *p;
    rb_chamber *cell;
    rb_face *face;
} duct;

static double seconds(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

static double packet(double x, double center, double sigma, double k, double amplitude)
{
    const double z = (x - center) / sigma;
    return amplitude * exp(-0.5 * z * z) * cos(k * (x - center));
}

static int init(duct *d, double dx, double temperature, double u, int order, int cells_per)
{
    memset(d, 0, sizeof *d);
    d->cells_per = cells_per;
    d->n = (int)lround(LENGTH / dx);
    if (d->n % cells_per) return 0;
    d->count = d->n / cells_per;
    d->dx = LENGTH / d->n;
    d->u = u;
    d->p = calloc((size_t)d->count, sizeof *d->p);
    d->cell = calloc((size_t)d->n, sizeof *d->cell);
    d->face = calloc((size_t)d->n, sizeof *d->face);
    if (!d->p || !d->cell || !d->face) return 0;
    for (int j = 0; j < d->count; j++) {
        if (rb_pipe_init(&d->p[j], d->cell + j * cells_per,
                         d->face + j * cells_per, cells_per, cells_per * d->dx,
                         0.04, PRESSURE, temperature, AIR, temperature, 0.0)) return 0;
        d->p[j].momentum_order = order;
    }
    for (int j = 0; j < d->count; j++)
        if (rb_pipe_onward(&d->p[j], &d->p[(j + 1) % d->count])) return 0;
    d->rho = d->cell[0].rho;
    d->sound = rb_chamber_sound_speed(d->cell);
    d->area = d->p[0].area;
    for (int j = 0; j < d->count; j++) {
        for (int q = 0; q < cells_per - 1; q++) d->p[j].face[q].mdot = d->rho * u * d->area;
        d->p[j].end_face[1].mdot = d->rho * u * d->area;
        for (int q = 0; q < cells_per; q++)
            d->p[j].kinetic[q] = 0.5 * d->p[j].cell[q].mass * u * u;
    }
    return 1;
}

static void release(duct *d) { free(d->p); free(d->cell); free(d->face); }

/* The forward flow is staggered half a time step ahead of cell pressure.
 * Include mean-flow density transport: delta(rho*u)=delta(p)*(1/c+U/c^2).
 * The Gaussian is narrowband but localized, with <exp(-32) edge amplitude. */
static void excite(duct *d, double temperature, double frequency, double amplitude, double dt)
{
    const double speed = d->sound + d->u, k = 2.0 * PI * frequency / speed;
    const double sigma = 0.5 * speed / frequency, center = 0.5 * LENGTH;
    for (int i = 0; i < d->n; i++) {
        rb_chamber *g = &d->cell[i];
        const double dp = packet((i + 0.5) * d->dx, center, sigma, k, amplitude);
        const double gamma = rb_chamber_gamma(g);
        rb_chamber_init(g, d->area * d->dx, PRESSURE + dp,
                        temperature * pow((PRESSURE + dp) / PRESSURE, (gamma - 1.0) / gamma), AIR);
    }
    for (int j = 0; j < d->count; j++) {
        rb_pipe *p = &d->p[j];
        for (int q = 0; q < d->cells_per; q++) {
            const int i = j * d->cells_per + q;
            const double dp = packet((i + 1.0) * d->dx, center + speed * 0.5 * dt, sigma, k, amplitude);
            rb_face *f = q == d->cells_per - 1 ? &p->end_face[1] : &p->face[q];
            f->mdot += d->area * dp * (1.0 / d->sound + d->u / (d->sound * d->sound));
        }
    }
    for (int j = 0; j < d->count; j++)
        for (int q = 0; q < d->cells_per; q++) {
            const double v = rb_pipe_cell_velocity(&d->p[j], q);
            d->p[j].kinetic[q] = 0.5 * d->p[j].cell[q].mass * v * v;
        }
}

static int step(duct *d, double dt)
{
    int mass_caps = 0;
    for (int j = 0; j < d->count; j++) rb_pipe_clear(&d->p[j], 0);
    for (int j = 0; j < d->count; j++) rb_pipe_flows(&d->p[j], 0, dt);
    for (int j = 0; j < d->count; j++)
        for (int q = 0; q < d->cells_per; q++) {
            const rb_face *f = q == d->cells_per - 1 ? &d->p[j].end_face[1] : &d->p[j].face[q];
            mass_caps += fabs(f->moved - f->mdot) > 1e-10 * fabs(f->mdot) + 1e-14;
        }
    for (int j = 0; j < d->count; j++) rb_pipe_integrate(&d->p[j], 0, dt);
    for (int j = 0; j < d->count; j++) rb_pipe_accelerate(&d->p[j], 0, dt);
    return mass_caps;
}

typedef struct { double ep, ev, re, im, centroid, sum, base_error, peak; int repairs, nonfinite, caps; } metrics;

static metrics measure(const duct *d, const duct *b, double frequency)
{
    metrics m = {0};
    const double k = 2.0 * PI * frequency / (d->sound + d->u);
    for (int j = 0; j < d->count; j++) {
        const rb_pipe *p = &d->p[j], *pb = &b->p[j];
        for (int q = 0; q < d->cells_per; q++) {
            const int i = j * d->cells_per + q, next = (i + 1) % d->n;
            const rb_face *f = q == d->cells_per - 1 ? &p->end_face[1] : &p->face[q];
            const rb_face *fb = q == d->cells_per - 1 ? &pb->end_face[1] : &pb->face[q];
            const double dp = d->cell[i].p - b->cell[i].p;
            const double drho = 0.5 * (d->cell[i].rho - b->cell[i].rho
                                      + d->cell[next].rho - b->cell[next].rho);
            const double du = ((f->mdot - fb->mdot) / d->area - d->u * drho) / d->rho;
            const double x = (i + 0.5) * d->dx;
            m.ep += dp * dp / (d->rho * d->sound * d->sound) * d->area * d->dx;
            m.ev += d->rho * du * du * d->area * d->dx;
            m.re += dp * cos(k * x) * d->dx;
            m.im -= dp * sin(k * x) * d->dx;
            m.centroid += x * dp * dp;
            m.sum += dp * dp;
            m.peak = fmax(m.peak, fabs(dp));
            m.repairs += d->cell[i].repairs + b->cell[i].repairs;
            m.nonfinite += !isfinite(dp + du + d->cell[i].T + b->cell[i].T);
            m.base_error = fmax(m.base_error, fabs(b->cell[i].p - PRESSURE));
            const rb_chamber *from = f->mdot >= 0 ? f->a : f->b;
            const double cap = RB_PIPE_MOST_SONIC * f->area * sqrt(1.4 * from->rho * from->p);
            m.caps += fabs(f->mdot) >= cap * (1.0 - 1e-9);
        }
    }
    if (m.sum > 0) m.centroid /= m.sum;
    return m;
}

static int run(double dx, double rate, double temperature, double u, double frequency,
               double amplitude, int order, int cells_per, int mutant, const char *group)
{
    duct d, b;
    if (!init(&d, dx, temperature, u, order, cells_per) ||
        !init(&b, dx, temperature, u, order, cells_per)) return 0;
    const double dt = 1.0 / rate, speed = d.sound + u, ppw = speed / frequency / d.dx;
    const double sigma = 0.5 * speed / frequency;
    const int steps = (int)lround(probe_duration / dt);
    const double elapsed = steps * dt;
    const double guard = 0.5 * LENGTH - 8.0 * sigma - (d.sound + fabs(u)) * elapsed;
    const double max_cfl = (d.sound + fabs(u)) * dt / d.dx;
    const double inertance_ratio = pow(RB_PIPE_WAVE_SPEED * dt, 2) / (2.0 * d.dx * d.dx);
    const int valid = guard > 0 && ppw >= 4.0 && max_cfl <= 0.8 && inertance_ratio < 1.0;
    if (!valid) {
        printf("%s,excluded,%d,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g",
               group,order,cells_per,mutant,temperature,u,frequency,amplitude,d.dx,rate,dt,d.sound,ppw,max_cfl,inertance_ratio,guard);
        for (int i=0;i<17;i++) printf(",0");
        putchar('\n');
        release(&d); release(&b); return 1;
    }
    excite(&d, temperature, frequency, amplitude, dt);
    const metrics first = measure(&d, &b, frequency);
    double maxbase = 0; int caps = 0, nonfinite = 0;
    const double start = seconds();
    for (int s = 0; s < steps; s++) {
        caps += step(&d, dt); caps += step(&b, dt);
        /* Deliberate harness control: erase the perturbation at the end.
         * Its reported zero retention must fail the analysis self-check. */
        if (s % 16 == 0 || s == steps - 1) {
            metrics mid = measure(&d, &b, frequency);
            maxbase = fmax(maxbase, mid.base_error);
            caps += mid.caps; nonfinite += mid.nonfinite;
        }
    }
    const double cpu = seconds() - start;
    if (mutant) {
        for (int i = 0; i < d.n; i++) d.cell[i] = b.cell[i];
        for (int j = 0; j < d.count; j++) {
            for (int q = 0; q < cells_per - 1; q++) d.p[j].face[q].mdot = b.p[j].face[q].mdot;
            d.p[j].end_face[1].mdot = b.p[j].end_face[1].mdot;
        }
    }
    const metrics last = measure(&d, &b, frequency);
    const double norm = first.re * first.re + first.im * first.im;
    double re = (last.re * first.re + last.im * first.im) / norm;
    double im = (last.im * first.re - last.re * first.im) / norm;
    const double angle = 2 * PI * frequency * elapsed;
    const double ar = re * cos(angle) - im * sin(angle), ai = re * sin(angle) + im * cos(angle);
    const double group_speed = (last.centroid - first.centroid) / elapsed;
    printf("%s,%s,%d,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%.9g,%.12g,%.12g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d,%.9g,%.9g\n",
           group,last.repairs || nonfinite || caps ? "failed" : "ok",order,cells_per,mutant,
           temperature,u,frequency,amplitude,d.dx,rate,dt,d.sound,ppw,max_cfl,inertance_ratio,guard,
           d.n,elapsed,first.ep+first.ev,last.ep+last.ev,(last.ep+last.ev)/(first.ep+first.ev),
           last.ep/first.ep,hypot(ar,ai),atan2(ai,ar)*180/PI,group_speed,100*(group_speed/speed-1),
           maxbase,last.peak,last.repairs,nonfinite,caps,cpu,cpu/elapsed);
    fflush(stdout);
    release(&d); release(&b); return 1;
}

int main(int argc, char **argv)
{
    const int quick = argc > 1 && !strcmp(argv[1], "--quick");
    const int controls = argc > 1 && !strcmp(argv[1], "--controls");
    const double grids[] = {.075,.0375,.01875,.009375};
    const double rates[] = {48000,96000,192000,384000};
    const double temperatures[] = {300,900}, flows[] = {-70,0,70};
    const double frequencies[] = {200,800,2000,4000};
    puts("group,status,order,cells_per,mutant,temperature_K,mean_flow_m_s,frequency_Hz,amplitude_Pa,dx_m,rate_Hz,dt_s,sound_m_s,points_per_wavelength,cfl,inertance_floor_ratio,seam_guard_m,cells,time_s,energy_initial_J,energy_final_J,energy_ratio,pressure_energy_ratio,carrier_magnitude,carrier_phase_error_deg,centroid_speed_m_s,speed_error_percent,base_pressure_error_Pa,final_peak_Pa,repairs,nonfinite,cap_observations,wall_seconds,wall_realtime_factor");
    if (controls) {
        const double bench_dx[] = {.075,.075,.0375,.0375,.01875,.01875,.009375,.009375};
        const double bench_rate[] = {48000,192000,48000,192000,96000,192000,192000,384000};
        for (int rep=0;rep<3;rep++) for (int i=0;i<8;i++)
            if (!run(bench_dx[i],bench_rate[i],900,70,800,10,2,64,0,"timing_repeat")) return 1;
        for (int duration=1;duration<=4;duration*=2) {
            probe_duration = DURATION * duration;
            for (int r=0;r<3;r++)
                if (!run(.009375,192000.0 * (1<<r),900,70,4000,5,2,64,0,"growth_check")) return 1;
        }
        return 0;
    }
    if (quick) {
        return !(run(.075,48000,300,70,800,10,2,64,0,"quick") &&
                 run(.01875,192000,300,0,800,10,2,64,0,"quick") &&
                 run(.009375,384000,900,0,200,10,2,64,0,"quick"));
    }
    for (int t=0;t<2;t++) for(int u=0;u<3;u++) for(int f=0;f<4;f++)
        for(int g=0;g<4;g++) for(int r=0;r<4;r++) {
            /* Conservative production mesh criterion; excluded combinations
             * are recorded but not interpreted as stable accuracy evidence. */
            if (RB_PIPE_WAVE_SPEED / rates[r] / grids[g] > RB_PIPE_COURANT + 1e-12) continue;
            if (!run(grids[g],rates[r],temperatures[t],flows[u],frequencies[f],10,2,64,0,"matrix")) return 1;
        }
    for(int t=0;t<2;t++) for(int u=0;u<3;u++) {
        if (!run(.075,48000,temperatures[t],flows[u],800,10,1,64,0,"donor_control")) return 1;
        if (!run(.01875,192000,temperatures[t],flows[u],2000,10,1,64,0,"donor_control")) return 1;
        if (!run(.01875,192000,temperatures[t],flows[u],800,5,2,64,0,"half_amplitude")) return 1;
        if (!run(.01875,192000,temperatures[t],flows[u],800,10,2,32,0,"partition_control")) return 1;
    }
    if (!run(.01875,192000,300,0,800,10,2,64,1,"erased_packet_mutant")) return 1;
    return 0;
}
