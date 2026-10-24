/*
 * pipe.c - the 1D pipe (see pipe.h).
 */
#include "pipe.h"
#include "diagnostics.h"

#include <math.h>
#include <string.h>

static const double PI = 3.14159265358979323846;

/* Prandtl number of air and exhaust gas, about 0.7: Pr^(-2/3) = 1.27. */
#define PR_FACTOR 1.27
/* How far beyond a pipe's open end the moving gas reaches: the end
 * correction, about 0.6 radius for a pipe end in open space. */
#define END_CORRECTION 0.3   /* diameters */

int rb_pipe_cells_for(double length, double cell, double dt)
{
    if (!(length > 0.0) || !(cell > 0.0)) return 1;
    int n = (int)floor(length / cell + 0.5);
    const double shortest = RB_PIPE_WAVE_SPEED * dt / RB_PIPE_COURANT;
    if (shortest > 0.0 && n > length / shortest) n = (int)floor(length / shortest);
    if (n > RB_PIPE_MAX_CELLS) n = RB_PIPE_MAX_CELLS;
    return (n < 1) ? 1 : n;
}

static void face_init(rb_face *f, rb_chamber *a, rb_chamber *b, double area, double length,
                      double friction)
{
    memset(f, 0, sizeof *f);
    f->a = a;
    f->b = b;
    f->area = area;
    f->length = length;
    f->per_area = 1.0 / area;
    f->per_length = 1.0 / length;
    f->friction = friction;
    f->pipe_end = -1;
}

int rb_pipe_init(rb_pipe *p, rb_chamber *cell, rb_face *face, int cells, double length,
                 double diameter, double p0, double T0, const double y[RB_NSPECIES],
                 double wall_temperature, double friction_factor)
{
    if (cells < 1 || cells > RB_PIPE_MAX_CELLS || !(length > 0.0) || !(diameter > 0.0)
        || !cell || (cells > 1 && !face))
        return -1;
    memset(p, 0, sizeof *p);
    p->momentum_order = 2;
    p->cell = cell;
    p->face = face;
    p->cells = cells;
    p->length = length;
    p->diameter = diameter;
    p->area = 0.25 * PI * diameter * diameter;
    p->per_area = 1.0 / p->area;
    p->dx = length / cells;
    p->wall_ratio = 4.0 * p->dx / diameter;
    p->friction_factor = friction_factor;
    p->stanton = friction_factor / 8.0 * PR_FACTOR;
    p->wall_temperature = wall_temperature;
    for (int k = 0; k < cells; k++) rb_chamber_init(&p->cell[k], p->area * p->dx, p0, T0, y);
    for (int k = 0; k + 1 < cells; k++)
        face_init(&p->face[k], &p->cell[k], &p->cell[k + 1], p->area, p->dx,
                  friction_factor * p->dx / diameter);
    return 0;
}

rb_chamber *rb_pipe_end_cell(rb_pipe *p, int end)
{
    return &p->cell[end ? p->cells - 1 : 0];
}

/* 1 if cell k is stepped in the fast pass: a fast end's cell (in a one-cell
 * pipe, either end's). */
static int cell_is_fast(const rb_pipe *p, int k)
{
    return (k == 0 && p->fast[0]) || (k == p->cells - 1 && p->fast[1]);
}

/* A face is moved in the fast pass if either side is fast; a slow side then
 * gets its gas at once. */
static void sort_face(rb_face *f, int fast_a, int fast_b)
{
    f->fast = fast_a || fast_b;
    f->direct_a = f->fast && !fast_a;
    f->direct_b = f->fast && !fast_b;
}

/* Works out which faces are moved in which pass. Called whenever an end
 * changes. */
static void sort_faces(rb_pipe *p)
{
    for (int k = 0; k + 1 < p->cells; k++)
        sort_face(&p->face[k], cell_is_fast(p, k), cell_is_fast(p, k + 1));
    for (int end = 0; end < 2; end++) {
        if (p->end[end].kind != RB_PIPE_JOINED) continue;
        const int cell = cell_is_fast(p, end ? p->cells - 1 : 0), chamber = p->end[end].fast_chamber;
        /* end 0's face runs from the chamber to the cell, end 1's the other way */
        if (end) sort_face(&p->end_face[end], cell, chamber);
        else sort_face(&p->end_face[end], chamber, cell);
    }
}

void rb_pipe_join(rb_pipe *p, int end, rb_chamber *chamber, double entry_loss, int fast_chamber)
{
    memset(&p->radiation[end ? 1 : 0], 0, sizeof p->radiation[0]);
    rb_pipe_end *e = &p->end[end ? 1 : 0];
    rb_face *f = &p->end_face[end ? 1 : 0];
    rb_chamber *cell = rb_pipe_end_cell(p, end);
    /* half a cell to the end, plus the end correction */
    const double length = 0.5 * p->dx + END_CORRECTION * p->diameter;
    if (end) face_init(f, cell, chamber, p->area, length, p->friction_factor * 0.5 * p->dx / p->diameter);
    else     face_init(f, chamber, cell, p->area, length, p->friction_factor * 0.5 * p->dx / p->diameter);
    f->entry = entry_loss;
    f->pipe_end = end ? 1 : 0;
    e->kind = RB_PIPE_JOINED;
    e->chamber = chamber;
    e->orifice = NULL;
    e->fast_chamber = fast_chamber;
    sort_faces(p);
}

void rb_pipe_restrict(rb_pipe *p, int end, double area)
{
    rb_face *f = &p->end_face[end ? 1 : 0];
    if (!(area > 0.0)) return;
    const double r = p->area / area;
    f->exit = (r > 1.0) ? r * r - 1.0 : 0.0;
    f->entry += (r > 1.0) ? (r - 1.0) * (r - 1.0) : 0.0;
}

int rb_pipe_radiate(rb_pipe *p, int end, double rho, double c)
{
    if (!p || end < 0 || end > 1 || p->end[end].kind != RB_PIPE_JOINED
        || !p->end[end].chamber->fixed || p->end_face[end].fast
        || p->end_face[end].packing > 0.0 || p->end_face[end].laminar > 0.0
        || p->end_face[end].exit > 0.0) return -1;
    rb_radiation_init(&p->radiation[end], 0.5 * p->diameter, rho, c);
    if (!p->radiation[end].enabled) return -1;
    rb_face *f = &p->end_face[end];
    /* The load already contains the exterior inertance. Keeping the old
     * end correction as well would count that moving gas twice. */
    f->length = 0.5 * p->dx;
    f->per_length = 1.0 / f->length;
    return 0;
}

void rb_pipe_resist(rb_pipe *p, int end, double laminar)
{
    p->end_face[end ? 1 : 0].laminar = (laminar > 0.0) ? laminar : 0.0;
}

int rb_pipe_onward(rb_pipe *a, rb_pipe *b)
{
    if (!a || !b || a == b || a->cells < 2 || b->cells < 2 || a->fast[1] || b->fast[0]) return -1;
    if (a->area > RB_PIPE_ONWARD_MAX_RATIO * b->area || b->area > RB_PIPE_ONWARD_MAX_RATIO * a->area)
        return -1;
    rb_face *f = &a->end_face[1];
    rb_chamber *from = rb_pipe_end_cell(a, 1), *to = rb_pipe_end_cell(b, 0);
    /* half of each cell, the narrower bore */
    const double area = (a->area < b->area) ? a->area : b->area;
    const double friction = 0.5 * (a->friction_factor * a->dx / a->diameter
                                 + b->friction_factor * b->dx / b->diameter);
    face_init(f, from, to, area, 0.5 * (a->dx + b->dx), friction);
    f->pipe_end = 1;
    a->end[1] = (rb_pipe_end){ .kind = RB_PIPE_ONWARD, .chamber = to, .pipe = b };
    b->end[0] = (rb_pipe_end){ .kind = RB_PIPE_ONWARD_FROM, .chamber = from, .pipe = a };
    sort_faces(a);
    sort_faces(b);
    return 0;
}

void rb_pipe_pack(rb_pipe *p, double resistance, double settle_time)
{
    const int on = resistance > 0.0 && settle_time > 0.0;
    for (int k = 0; k + 1 < p->cells; k++) {
        rb_face *f = &p->face[k];
        f->packing = on ? resistance * f->length : 0.0;
        f->settle_time = on ? settle_time : 0.0;
        f->settled = f->mdot;
    }
}

void rb_pipe_attach(rb_pipe *p, int end, rb_chamber *chamber, const rb_orifice *o)
{
    rb_pipe_end *e = &p->end[end ? 1 : 0];
    const rb_chamber *cell = rb_pipe_end_cell(p, end);
    e->kind = RB_PIPE_ORIFICE;
    e->chamber = chamber;
    e->orifice = o;
    /* positive orifice flow runs a -> b: along the pipe if it leaves end 1's
     * cell or enters end 0's */
    e->sign = ((end && o->a == cell) || (!end && o->b == cell)) ? 1.0 : -1.0;
    e->fast_chamber = 0;
    sort_faces(p);
}

void rb_pipe_set_fast(rb_pipe *p, int end)
{
    p->fast[end ? 1 : 0] = 1;
    sort_faces(p);
}

/* The mass flow (kg/s) along the pipe at an end: its face's, its orifice's
 * (turned to run along the pipe), the face from the pipe it goes on from, or
 * none when closed. */
static double end_flow(const rb_pipe *p, int end)
{
    const rb_pipe_end *e = &p->end[end];
    if (e->kind == RB_PIPE_JOINED || e->kind == RB_PIPE_ONWARD) return p->end_face[end].mdot;
    if (e->kind == RB_PIPE_ONWARD_FROM) return e->pipe->end_face[1].mdot;
    if (e->kind == RB_PIPE_ORIFICE) return e->sign * e->orifice->mdot;
    return 0.0;
}

/* The mass flow (kg/s, along the pipe) through each cell's sides: flow[k]
 * through cell k's side towards end 0, flow[k + 1] towards end 1. */
static void side_flows(const rb_pipe *p, double flow[RB_PIPE_MAX_CELLS + 1])
{
    flow[0] = end_flow(p, 0);
    for (int k = 0; k + 1 < p->cells; k++) flow[k + 1] = p->face[k].mdot;
    flow[p->cells] = end_flow(p, 1);
}

/* The flow through cell k's side towards end 0 (side 0) or end 1 (side 1). */
static double side_flow(const rb_pipe *p, int k, int side)
{
    if (side == 0) return (k > 0) ? p->face[k - 1].mdot : end_flow(p, 0);
    return (k < p->cells - 1) ? p->face[k].mdot : end_flow(p, 1);
}

/* The velocity of a cell's gas, given the flows through its two sides
 * (0 in an empty cell, whose per_rho is 0). */
static double velocity(const rb_pipe *p, int k, double in, double out)
{
    return 0.5 * (in + out) * p->cell[k].per_rho * p->per_area;
}

/* Kinetic energy per kg of a cell's gas (J/kg). */
static double kinetic_energy(const rb_pipe *p, int k)
{
    const double v = velocity(p, k, side_flow(p, k, 0), side_flow(p, k, 1));
    if(p->diag && v!=v) rb_diag_note(p->diag,RB_D_BIT(RB_D_BAD_FLOW));
    return (v == v) ? 0.5 * v * v : 0.0;
}

double rb_pipe_cell_flow(const rb_pipe *p, int cell)
{
    return 0.5 * (side_flow(p, cell, 0) + side_flow(p, cell, 1));
}

double rb_pipe_cell_velocity(const rb_pipe *p, int cell)
{
    return velocity(p, cell, side_flow(p, cell, 0), side_flow(p, cell, 1));
}

/* The fast pass only ever touches the cells at the ends. */
static int fast_cells(const rb_pipe *p, int cell[2])
{
    int n = 0;
    if (p->fast[0]) cell[n++] = 0;
    if (p->fast[1] && !(p->cells == 1 && n)) cell[n++] = p->cells - 1;
    return n;
}

void rb_pipe_clear(rb_pipe *p, int fast)
{
    if (fast) {
        int cell[2];
        for (int j = fast_cells(p, cell) - 1; j >= 0; j--) rb_chamber_clear_rates(&p->cell[cell[j]]);
        return;
    }
    for (int k = 0; k < p->cells; k++)
        if (!cell_is_fast(p, k)) rb_chamber_clear_rates(&p->cell[k]);
}

/* Moves gas through a face with its current flow, carrying the upstream
 * chamber's gas and energy, its enthalpy and its kinetic energy (ke_a or ke_b,
 * J/kg), but never more than half of what's there in one step. A side
 * marked direct gets it into its masses at once. */
static void move_gas(rb_face *f, double dt, int direct, double ke_a, double ke_b)
{
    const int forward = f->mdot >= 0.0;
    rb_chamber *up = forward ? f->a : f->b, *down = forward ? f->b : f->a;
    const int direct_up = direct && (forward ? f->direct_a : f->direct_b);
    const int direct_down = direct && (forward ? f->direct_b : f->direct_a);
    double m = fabs(f->mdot);
    const double requested = f->mdot*dt;
    uint64_t reasons=0;
    if (!(m < INFINITY)) {m = 0.0;reasons|=RB_D_BIT(RB_D_BAD_FLOW);} /* NaN or inf: none */
    if (!(up->mass > 0.0)) {m = 0.0;reasons|=RB_D_BIT(RB_D_EMPTY);}
    else if (m * dt > 0.5 * up->mass) {m = 0.5 * up->mass / dt;reasons|=RB_D_BIT(RB_D_MASS_CAP);}
    f->moved = forward ? m : -m;
    f->moved_h = up->h + (forward ? ke_a : ke_b);
    for (int s = 0; s < RB_NSPECIES; s++) f->moved_y[s] = up->y[s];
    if(f->diag) rb_diag_transfer(f->diag,reasons,requested,f->moved*dt,f->moved_y,f->moved_h);
    if (m == 0.0) return;
    if (direct_down) rb_chamber_add_mass(down, m * dt, f->moved_y, f->moved_h);
    else rb_chamber_add_flow(down, m, f->moved_y, f->moved_h);
    if (direct_up) rb_chamber_add_mass(up, -m * dt, f->moved_y, f->moved_h);
    else rb_chamber_add_flow(up, -m, f->moved_y, f->moved_h);
}

/* A cell's own heat: its gas's kinetic energy comes out of its internal
 * energy (what speeds it up, or slows it down, cools it or warms it:
 * friction and the jets lost in the chambers slow the gas, and so heat it),
 * and the walls warm or cool it, q = St cp |mdot| / A * (pi D dx) (T_wall - T),
 * the cell's mass flow taken as the mean of its two sides' sizes. per_dt is
 * 1 / dt. */
static void cell_heat(rb_pipe *p, int k, double ke, double in, double out, double dt,
                      double per_dt)
{
    rb_chamber *c = &p->cell[k];
    if (!(c->mass > 0.0)) return;
    const double kinetic = c->mass * ke;
    rb_chamber_add_heat(c, -(kinetic - p->kinetic[k]) * per_dt);
    p->kinetic[k] = kinetic;
    const double flow = 0.5 * (fabs(in) + fabs(out));
    const double q = p->stanton * (c->cv + c->gas.R) * flow * p->wall_ratio
                   * (p->wall_temperature - c->T);
    if (!(q == q) || q == 0.0) {
        if(p->diag && q!=q) rb_diag_note(p->diag,RB_D_BIT(RB_D_HEAT_RESET));
        return;
    }
    rb_chamber_add_heat(c, q);
    p->heat += q * dt;
}

/* The fast pass: the faces at the ends that touch something fast. */
static void fast_flows(rb_pipe *p, double dt)
{
    const int n = p->cells;
    for (int end = 0; end < 2; end++) {
        rb_face *f = &p->end_face[end];
        if (p->end[end].kind == RB_PIPE_JOINED && f->fast) {
            const double ke = kinetic_energy(p, end ? n - 1 : 0);
            move_gas(f, dt, 1, end ? ke : 0.0, end ? 0.0 : ke);   /* the chamber's gas is still */
        }
    }
    if (n >= 2 && p->face[0].fast)
        move_gas(&p->face[0], dt, 1, kinetic_energy(p, 0), kinetic_energy(p, 1));
    if (n >= 3 && p->face[n - 2].fast)
        move_gas(&p->face[n - 2], dt, 1, kinetic_energy(p, n - 2), kinetic_energy(p, n - 1));
}

void rb_pipe_flows(rb_pipe *p, int fast, double dt)
{
    if (fast) {
        fast_flows(p, dt);
        p->since += dt;   /* the slow cells' rates have run a step longer */
        return;
    }
    p->since = 0.0;       /* they start now */
    const int n = p->cells;
    double flow[RB_PIPE_MAX_CELLS + 1], ke[RB_PIPE_MAX_CELLS];
    side_flows(p, flow);
    for (int k = 0; k < n; k++) {
        const double v = velocity(p, k, flow[k], flow[k + 1]);
        ke[k] = (v == v) ? 0.5 * v * v : 0.0;
    }
    for (int k = 0; k + 1 < n; k++)
        if (!p->face[k].fast) move_gas(&p->face[k], dt, 0, ke[k], ke[k + 1]);
    for (int end = 0; end < 2; end++)   /* the chambers' gas is still */
        if (p->end[end].kind == RB_PIPE_JOINED && !p->end_face[end].fast) {
            move_gas(&p->end_face[end], dt, 0, end ? ke[n - 1] : 0.0, end ? 0.0 : ke[0]);
            if (p->radiation[end].enabled) {
                rb_chamber *cell = rb_pipe_end_cell(p, end);
                const double power = p->radiation[end].pressure * p->end_face[end].moved * cell->per_rho;
                rb_chamber_add_heat(cell, -power);
                p->radiated += dt * power;
            }
        }
    if (p->end[1].kind == RB_PIPE_ONWARD)   /* the next pipe's gas moves */
        move_gas(&p->end_face[1], dt, 0, ke[n - 1], kinetic_energy(p->end[1].pipe, 0));
}

void rb_pipe_contents(const rb_pipe *p, double *mass, double *energy, double species[RB_NSPECIES])
{
    double m = 0.0, e = 0.0, s[RB_NSPECIES] = { 0.0 };
    for (int k = 0; k < p->cells; k++) {
        const rb_chamber *c = &p->cell[k];
        m += c->mass;
        e += c->U + p->kinetic[k];
        for (int j = 0; j < RB_NSPECIES; j++) s[j] += c->m[j];
    }
    if (mass) *mass = m;
    if (energy) *energy = e;
    if (species)
        for (int j = 0; j < RB_NSPECIES; j++) species[j] = s[j];
}

/* Each cell's own heat (cell_heat) goes in as it's integrated, so the books
 * (rb_pipe_contents, heat) change with its energy, even when the slow pass
 * collected its flows a block of steps before. */
void rb_pipe_integrate(rb_pipe *p, int fast, double dt)
{
    const double per_dt = 1.0 / dt;
    if (fast) {
        int cell[2];
        for (int j = fast_cells(p, cell) - 1; j >= 0; j--) {
            const int k = cell[j];
            cell_heat(p, k, kinetic_energy(p, k), side_flow(p, k, 0), side_flow(p, k, 1), dt,
                      per_dt);
            rb_chamber_integrate(&p->cell[k], dt);
        }
        return;
    }
    const int n = p->cells;
    double flow[RB_PIPE_MAX_CELLS + 1];
    side_flows(p, flow);
    for (int k = 0; k < n; k++) {
        if (cell_is_fast(p, k)) continue;
        const double v = velocity(p, k, flow[k], flow[k + 1]);
        cell_heat(p, k, (v == v) ? 0.5 * v * v : 0.0, flow[k], flow[k + 1], dt, per_dt);
    }
    const int first = p->fast[0] ? 1 : 0;
    const int last = n - (p->fast[1] ? 1 : 0);
    rb_chamber_integrate_many(p->cell + first, last > first ? last - first : 0, dt);
    p->since = 0.0;       /* their rates are in */
}

/* A face's new flow: the pressure difference along it (a to b) less the
 * losses (`heads` velocity heads at the density rho, its laminar loss and its
 * packing's), plus the momentum flowing in at a's end less what flows out at
 * b's. The column is never taken shorter than `shortest`, which keeps a pipe
 * shorter than a stable cell stable at a slow step (pipe.h:
 * rb_pipe_cells_for). The laminar loss, A laminar u = laminar mdot / rho, and
 * the packing's, packing (mdot - settled) / rho, are taken at the new flow
 * (implicitly), so they can't overshoot however strong they are. Last, the
 * flow is held under RB_PIPE_MOST_SONIC times the sonic flow of the gas it
 * comes from (pipe.h). */
static void push(rb_face *f, double heads, double rho, double flux_a, double flux_b,
                 double shortest, double since, double dt)
{
    const double old=f->mdot;
    uint64_t reasons=0;
    if(f->diag && !(rho>1e-6)) reasons|=RB_D_BIT(RB_D_DENSITY_FLOOR);
    if(f->diag && f->length<shortest) reasons|=RB_D_BIT(RB_D_LENGTH_FLOOR);
    /* a slow side of a fast face (pipe.h, two rates): the pressure it has by now */
    const double dp = (f->direct_a ? rb_chamber_pressure_now(f->a, since) : f->a->p)
                    - (f->direct_b ? rb_chamber_pressure_now(f->b, since) : f->b->p);
    const double r = (rho > 1e-6) ? rho : 1e-6, per_r = 1.0 / r;
    /* The packing's loss is on the swing that's left once the settled flow
     * has followed the new flow by `lag` of the way (backward Euler, which
     * never overshoots): (1 - lag) (mdot - settled). */
    const double lag = (f->packing > 0.0) ? dt / (f->settle_time + dt) : 0.0;
    const double packing = f->packing * (1.0 - lag);
    /* the heads at u = mdot / (rho A): A heads rho u|u| / 2 = heads mdot|mdot| / (2 rho A) */
    const double force = f->area * dp - heads * 0.5 * f->mdot * fabs(f->mdot) * per_r * f->per_area
                       + flux_a - flux_b + packing * f->settled * per_r;
    const double per_length = (f->length > shortest) ? f->per_length : 1.0 / shortest;
    double mdot = f->mdot + dt * force * per_length;
    const double drag = f->laminar + packing;   /* Pa per m/s, taken at the new flow */
    if (drag > 0.0) mdot /= 1.0 + dt * drag * per_r * per_length;
    const double requested=mdot;
    /* never past RB_PIPE_MOST_SONIC times the sonic flow of the gas it takes */
    const rb_chamber *from = (mdot >= 0.0) ? f->a : f->b;
    const double most2 = RB_PIPE_MOST_SONIC * RB_PIPE_MOST_SONIC * 1.4 * from->rho * from->p
                       * f->area * f->area;
    if (mdot * mdot > most2) {
        mdot = (mdot > 0.0) ? sqrt(most2) : -sqrt(most2);
        reasons|=RB_D_BIT(RB_D_SONIC_CAP);
    }
    if(f->diag && !(mdot==mdot && fabs(mdot)<1e6)) reasons|=RB_D_BIT(RB_D_BAD_FLOW);
    f->mdot = (mdot == mdot && fabs(mdot) < 1e6) ? mdot : 0.0;   /* NaN: start from rest */
    if(f->diag) rb_diag_impulse(f->diag,reasons,(requested-old)/per_length,
                              (f->mdot-old)/per_length,1/per_length,r);
    f->settled += lag * (f->mdot - f->settled);
}

/* Limited linear reconstruction of the transported momentum density at a
 * cell centre, from the staggered faces. Minmod falls back to donor-cell
 * transport at extrema and boundaries. A single flux is shared by both
 * adjacent momentum volumes. Mass/species/enthalpy transport is unchanged.
 * This reduces artificial damping of acoustic detail in a moving stream;
 * it is not a second-order discretisation of the complete gas system.
 * Derivation and limits: docs/stage-S2b.md. */
static double momentum_flux(const rb_pipe *p, int k, double in, double out)
{
    const double mdot = 0.5 * (in + out);
    double transported = (mdot >= 0.0) ? in : out;
    if (p->momentum_order == 2) {
        double left = 0.0, right = 0.0;
        if (mdot >= 0.0 && k > 0) {
            left = in - side_flow(p, k - 1, 0);
            right = out - in;
        } else if (mdot < 0.0 && k + 1 < p->cells) {
            left = out - in;
            right = side_flow(p, k + 1, 1) - out;
        }
        if ((left > 0.0 && right > 0.0) || (left < 0.0 && right < 0.0)) {
            const double slope = copysign(fmin(fabs(left), fabs(right)), left);
            transported += ((mdot >= 0.0) ? 0.5 : -0.5) * slope;
            if(p->diag) rb_diag_note(p->diag,RB_D_BIT(RB_D_RECONSTRUCT));
        } else {
            if(p->diag) rb_diag_note(p->diag,RB_D_BIT(RB_D_DONOR));
        }
    }
    return mdot * transported * p->cell[k].per_rho * p->per_area;
}

/* An end's face: from a chamber's still gas into the pipe the gas loses
 * (1 + entry) velocity heads speeding up; out of it, its jet's speed is lost
 * in the chamber (and `exit` more through a restriction). Friction over the
 * half cell either way. */
static void push_end(rb_pipe *p, int end, double shortest, double dt)
{
    rb_face *f = &p->end_face[end];
    const rb_chamber *chamber = end ? f->b : f->a, *cell = end ? f->a : f->b;
    const int inward = end ? (f->mdot < 0.0) : (f->mdot > 0.0);
    const double heads = f->friction + (inward ? 1.0 + f->entry : f->exit);
    push(f, heads, inward ? chamber->rho : cell->rho, 0.0, 0.0, shortest, p->since, dt);
    rb_radiation *r = &p->radiation[end];
    if (r->enabled) {
        double direct, history;
        rb_radiation_linear(r, dt, &direct, &history);
        const double per_length = 1.0 / fmax(f->length, shortest);
        const double density = fmax(cell->rho, 1e-6);
        const double previous=f->mdot;
        f->mdot = (f->mdot - dt * per_length * f->area * history)
                / (1.0 + dt * per_length * direct / density);
        const double requested=f->mdot;
        const rb_chamber *from = f->mdot >= 0.0 ? f->a : f->b;
        const double cap = RB_PIPE_MOST_SONIC * f->area * sqrt(1.4 * from->rho * from->p);
        f->mdot = fmax(-cap, fmin(cap, f->mdot));
        if(f->diag)
            rb_diag_impulse(f->diag,requested!=f->mdot ?
                            RB_D_BIT(isfinite(requested) ? RB_D_SONIC_CAP:RB_D_BAD_FLOW):0,
                            (requested-previous)/per_length,(f->mdot-previous)/per_length,
                            1/per_length,density);
        rb_radiation_step(r, f->mdot / (density * f->area));
    }
}

/* Face k, between cells k and k + 1. */
static void push_face(rb_pipe *p, int k, double flux_a, double flux_b, double shortest, double dt)
{
    rb_face *f = &p->face[k];
    push(f, f->friction, 0.5 * (f->a->rho + f->b->rho), flux_a, flux_b, shortest, p->since, dt);
}

/* The face on into the next pipe: like one of the pipe's own, with the next
 * pipe's first cell's momentum beyond it, and a narrowing's loss either way:
 * the jet through it spreads out again, (A / A_e - 1)^2 velocity heads
 * (rb_pipe_restrict keeps (A / A_e)^2 - 1 in `exit`). */
static void push_onward(rb_pipe *p, double shortest, double dt)
{
    rb_face *f = &p->end_face[1];
    const rb_pipe *b = p->end[1].pipe;
    const int n = p->cells;
    const double r = sqrt(1.0 + f->exit);
    push(f, f->friction + (r - 1.0) * (r - 1.0), 0.5 * (f->a->rho + f->b->rho),
         momentum_flux(p, n - 1, side_flow(p, n - 1, 0), side_flow(p, n - 1, 1)),
         momentum_flux(b, 0, side_flow(b, 0, 0), side_flow(b, 0, 1)), shortest, p->since, dt);
}

void rb_pipe_accelerate(rb_pipe *p, int fast, double dt)
{
    const int n = p->cells;
    if(p->diag) for(int k=0;k<n;k++) {
        if(cell_is_fast(p,k)!=!!fast) continue;
        const double cfl=(rb_chamber_sound_speed(&p->cell[k])+fabs(rb_pipe_cell_velocity(p,k)))*dt/p->dx;
        if(!isfinite(cfl)) p->diag->invalid++;
        else if(cfl>p->diag->max_cfl) p->diag->max_cfl=cfl;
        if(cfl>1) rb_diag_note(p->diag,RB_D_BIT(RB_D_CFL));
    }
    /* A column of length L between a cell of length dx and a big volume rings
     * at c / sqrt(L dx), which a step dt follows only below 2 / dt: so L must
     * be over (c dt)^2 / (4 dx). Twice that, at the fastest waves. Only a pipe
     * shorter than a stable cell (below 2.6 cm at 48 kHz) needs it. */
    const double wave = RB_PIPE_WAVE_SPEED * dt;
    const double shortest = wave * wave / (2.0 * p->dx);
    if (fast) {   /* the faces at the ends that touch something fast */
        for (int end = 0; end < 2; end++)
            if (p->end[end].kind == RB_PIPE_JOINED && p->end_face[end].fast)
                push_end(p, end, shortest, dt);
        for (int j = 0; j < 2; j++) {
            const int k = j ? n - 2 : 0;
            if (k < 0 || (j && k == 0) || !p->face[k].fast) continue;
            push_face(p, k, momentum_flux(p, k, side_flow(p, k, 0), side_flow(p, k, 1)),
                      momentum_flux(p, k + 1, side_flow(p, k + 1, 0), side_flow(p, k + 1, 1)),
                      shortest, dt);
        }
        return;
    }
    double flux[RB_PIPE_MAX_CELLS];   /* N: momentum through each cell's centre */
    double flow[RB_PIPE_MAX_CELLS + 1];
    side_flows(p, flow);
    for (int k = 0; k < n; k++) flux[k] = momentum_flux(p, k, flow[k], flow[k + 1]);
    for (int k = 0; k + 1 < n; k++)
        if (!p->face[k].fast) push_face(p, k, flux[k], flux[k + 1], shortest, dt);
    for (int end = 0; end < 2; end++)
        if (p->end[end].kind == RB_PIPE_JOINED && !p->end_face[end].fast) push_end(p, end, shortest, dt);
    if (p->end[1].kind == RB_PIPE_ONWARD) push_onward(p, shortest, dt);
}
