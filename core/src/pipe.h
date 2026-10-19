/*
 * pipe.h - a 1D pipe: an intake runner, an exhaust header, a tailpipe, with
 * pressure waves running up and down it (stage 10).
 *
 * The pipe is cut into cells, each a chamber (chamber.h) with its own
 * pressure, temperature and mix of gases, and neighbouring cells are joined
 * by faces. A face is stage 9's lumped runner made short: a column of gas
 * a cell long whose mass flow is a state of its own. The pressure difference
 * across it speeds it up, friction slows it, and it carries momentum along:
 *
 *   d(mdot)/dt = [ A (p_L - p_R) + F_L - F_R - K A rho u|u| / 2 ] / dx
 *
 *   F    the flux of momentum through a cell, mdot u, with u its upstream
 *        face's velocity ("donor cell")
 *   K    wall friction over the face's length, f dx / D (Darcy factor f)
 *
 * Each face carries its upstream cell's gas into the next: its composition,
 * and its energy, enthalpy plus kinetic energy (h + u^2/2). A cell's internal
 * energy pays for its gas's kinetic energy, so gas speeding up cools, and the
 * speed friction takes away comes back as heat. Mass, every species and the
 * total energy are conserved exactly. (Without the kinetic energy, waves ride
 * a flow about 1.2 u fast instead of u.) This is a staggered grid, as in 1D
 * engine codes such as GT-Power: masses and energies at the cells' centres,
 * flows at the faces between them. Stepped like stage 9's runner (move the
 * gas with the faces' flows, integrate the cells, then update the flows from
 * the new pressures: symplectic Euler), a pressure wave crosses the cells at
 * the speed of sound with hardly any numerical damping. It's stable while a
 * wave crosses less than a cell per step, (c + |u|) dt < dx.
 *
 * Its ends:
 *   - closed: a wall, where a wave comes back as it went;
 *   - joined to a chamber (the plenum, a collector, a muffler, the open air)
 *     through a face half a cell long plus the end correction (0.3 D, where
 *     the moving air really ends). Gas flowing in from the chamber's still
 *     gas speeds up, which takes (1 + K_in) velocity heads of pressure (K_in:
 *     the entry loss); flowing out, its jet's speed is lost in the chamber,
 *     which holds the end at the chamber's pressure. So a pressure wave comes
 *     back from an open end inverted. A restriction at the end (a catalyst,
 *     a muffler's baffles), an effective area A_e in a pipe of area A, costs
 *     (A / A_e)^2 - 1 more velocity heads flowing out (the jet through it is
 *     faster) and 1 + (A / A_e - 1)^2 flowing in (speeding up through it,
 *     then the Borda-Carnot loss as it spreads into the pipe). A catalyst's
 *     narrow channels, or a baffle's perforations, also cost a laminar loss,
 *     a pressure drop in proportion to the speed: unlike the velocity heads,
 *     which vanish for small flows, it damps even the smallest swings of the
 *     gas;
 *   - joined through an orifice (a valve, the catalyst): its owner computes
 *     the orifice's flow (flow.h) and the pipe reads it, for the momentum it
 *     brings;
 *   - going straight on into another pipe (stage 12: a straight pipe after
 *     the catalyst, a tailpipe after a straight-through muffler). The face
 *     between their end cells is like one between two of a pipe's own: half
 *     of each cell long, the gas's momentum on both sides, nothing lost but
 *     the walls' friction (and a narrowing's spreading out again,
 *     (A / A_e - 1)^2 velocity heads). Joined to the next pipe's end cell
 *     as to a volume instead, the gas would lose its speed there and pay
 *     to get it back: two velocity heads, 7.5 kPa more back pressure at the
 *     four's full power.
 *
 * The walls: heat flows between the gas and the pipe's wall (at a fixed
 * temperature) by Reynolds' analogy, h = St rho |u| cp with Colburn's
 * St = (f / 8) Pr^(-2/3).
 *
 * Packing (stage 12): a straight-through muffler is a perforated pipe in a
 * can of glass or steel wool. The gas flows straight through, but a sound
 * wave's pressure pushes gas in and out through the holes, and the wool's
 * fibres take its energy. Along the pipe that's a laminar loss on the flow's
 * swings only, what's faster than a settling time tau:
 *
 *   a face's loss = R dx (mdot - mdot_settled) / rho    (R: Pa per m/s per metre)
 *
 * mdot_settled following mdot with tau's lag. It takes nothing from the
 * steady flow (no back pressure), it damps fast swings by R / (2 rho c)
 * nepers a metre, and slow ones less, as (omega tau)^2 below 1 / tau: the
 * same shape as the loss of the perforations and the wool behind them (a
 * resistance in series with the can's compliance). It isn't the can, though:
 * below 1 / tau the settled flow drags a swing along with it, as if the gas
 * were R tau / (1 + (omega tau)^2) per m^3 heavier (the can would make room
 * instead, as if the pipe were wider). Keep R tau under the gas's density.
 *
 * Two rates. A pipe's end cell can be fast, stepped with the cylinders every
 * physics step, while the rest of the pipe is stepped every few steps (the
 * engine runs its pipes at about 48 kHz: a wave crosses a 5 cm cell in about
 * three of those steps). A slow step is a block of fast ones:
 *   - at its start the slow pass collects the slow cells' rates: the gas
 *     through the faces between slow cells;
 *   - every physics step the fast pass moves the gas through the faces that
 *     touch a fast cell or chamber and integrates the fast cells; gas
 *     reaching a slow cell goes straight into its masses and energy
 *     (rb_chamber_add_mass);
 *   - at its end the slow cells are integrated over the whole block, with
 *     the walls' heat, and the faces between them get their new flows;
 *   - and then, every physics step, the fast pass's faces get theirs. Where
 *     one side is a slow cell, they see the pressure it has by now
 *     (rb_chamber_pressure_now): the gas it has taken or given, and its
 *     rates running since the block began. Seeing the pressure of its last
 *     slow step instead, an intake port lags its runner, and the engine's
 *     breathing changes with the length of the step.
 * A pipe with no fast ends is simply stepped at one rate, the slow one.
 */
#ifndef RB_PIPE_H
#define RB_PIPE_H

#include "chamber.h"
#include "flow.h"

/* The most cells one pipe can have. */
#define RB_PIPE_MAX_CELLS 64

/* A face: a column of gas between two chambers, moving as one. Positive flow
 * goes from a to b, which along a pipe is from its end 0 to its end 1. */
typedef struct rb_face {
    rb_chamber *a, *b;
    double area;        /* m^2                                                 */
    double length;      /* m: the moving column's                              */
    double friction;    /* K: velocity heads lost to the walls along it        */
    double entry;       /* an end's face: velocity heads lost flowing into the
                           pipe, on top of the dynamic head (0 between cells)  */
    double exit;        /* and flowing out of it, on top of the jet's dynamic
                           head (0 for a plain end)                            */
    double laminar;     /* Pa per m/s of the pipe's gas, either way: a laminar
                           loss (0 but at a catalyst or a muffler's baffle)    */
    double packing;     /* Pa per m/s of the flow's swings: a muffler's packing
                           (0 elsewhere) ...                                   */
    double settle_time; /* ... which lets through what's slower than this (s) */
    double settled;     /* kg/s: the flow's slow part, lagging mdot by that    */
    int    pipe_end;    /* -1 between two cells; 0 or 1: the face at that end,
                           with the chamber on its a side (end 0) or b (end 1) */
    int    fast;        /* moved in the fast pass (it touches something fast)  */
    int    direct_a;    /* in the fast pass, a slow chamber on that side gets  */
    int    direct_b;    /* its gas at once (rb_chamber_add_mass)               */
    double mdot;        /* kg/s: the column's flow now                         */
    double moved;       /* kg/s: what it moved in its last pass (signed; under
                           mdot only if that would have emptied a chamber)     */
    double moved_h;     /* J/kg the moved gas carried: enthalpy + kinetic      */
    double moved_y[RB_NSPECIES];   /* and its composition                      */
} rb_face;

enum { RB_PIPE_CLOSED = 0, RB_PIPE_JOINED, RB_PIPE_ORIFICE, RB_PIPE_ONWARD, RB_PIPE_ONWARD_FROM };

struct rb_pipe;

typedef struct rb_pipe_end {
    int         kind;       /* RB_PIPE_CLOSED, _JOINED (a face to a chamber),
                               _ORIFICE, _ONWARD (end 1: going on into another
                               pipe, its face this pipe's) or _ONWARD_FROM (end 0:
                               going on from another pipe's end 1)               */
    rb_chamber *chamber;    /* what's beyond the end (NULL when closed): the
                               other pipe's end cell when going on               */
    struct rb_pipe *pipe;   /* the other pipe, when going on                     */
    const rb_orifice *orifice;   /* the orifice, for an _ORIFICE end             */
    double      sign;       /* +1 if the orifice's positive flow runs from end 0
                               towards end 1 along the pipe, -1 if against it    */
    int         fast_chamber;    /* the chamber is stepped every physics step    */
} rb_pipe_end;

/* A pipe. Its cells and the faces between them live in its owner's storage
 * (the engine keeps them all in one pool); the faces point at the cells, so
 * neither may move once the pipe is set up. */
typedef struct rb_pipe {
    int    cells;                       /* 1..RB_PIPE_MAX_CELLS                    */
    double length, diameter;            /* m                                       */
    double area, dx;                    /* m^2; m: each cell's length              */
    double friction_factor;             /* Darcy f                                 */
    double stanton;                     /* the walls' St (Colburn's from f; 0 for
                                           walls that keep their heat)             */
    double wall_temperature;            /* K                                       */
    int    fast[2];                     /* the cell at that end is fast            */
    rb_chamber *cell;                   /* cell[0..cells-1], from end 0 to end 1   */
    rb_face    *face;                   /* face[k] joins cell k and k + 1          */
    rb_face     end_face[2];            /* used by _JOINED ends                    */
    rb_pipe_end end[2];
    double kinetic[RB_PIPE_MAX_CELLS];  /* J: each cell's gas's kinetic energy, as
                                           its internal energy has paid for it     */
    double heat;                        /* J the walls have given the gas since
                                           rb_pipe_init (< 0: the gas lost heat)   */
    double since;                       /* s the slow cells' rates have run since
                                           they were collected (two rates)         */
} rb_pipe;

/* The cells for a pipe of `length` (m): about one every `cell` metres, from
 * 1 to RB_PIPE_MAX_CELLS, but never shorter than a wave travels in `dt` (s)
 * at RB_PIPE_WAVE_SPEED, divided by the safety factor RB_PIPE_COURANT. */
#define RB_PIPE_WAVE_SPEED  1000.0   /* m/s: c + |u| of the hottest, fastest gas   */
#define RB_PIPE_COURANT     0.8
int  rb_pipe_cells_for(double length, double cell, double dt);

/* A pipe of `cells` cells (1..RB_PIPE_MAX_CELLS), full of gas of composition y
 * at p and T, still, with both ends closed. `cell` must have room for `cells`
 * chambers and `face` for cells - 1 faces. Returns 0, or -1 for a bad size. */
int  rb_pipe_init(rb_pipe *p, rb_chamber *cell, rb_face *face, int cells, double length,
                  double diameter, double p0, double T0, const double y[RB_NSPECIES],
                  double wall_temperature, double friction_factor);

/* Joins end `end` (0 or 1) to a chamber through a face; entry_loss is K_in.
 * fast_chamber: the chamber is stepped every physics step. */
void rb_pipe_join(rb_pipe *p, int end, rb_chamber *chamber, double entry_loss, int fast_chamber);

/* Narrows a joined end to an effective area (Cd included, m^2): see above. */
void rb_pipe_restrict(rb_pipe *p, int end, double area);

/* Adds a laminar loss at a joined end: `laminar` Pa per m/s of the gas in
 * the pipe, whichever way it flows. */
void rb_pipe_resist(rb_pipe *p, int end, double laminar);

/* Joins end 1 of pipe a straight on to end 0 of pipe b (see above): the face
 * between them is a's end 1 face, which can then be narrowed
 * (rb_pipe_restrict) or resisted (rb_pipe_resist) like any of a's end faces.
 * Both end cells must be slow, and neither pipe a one-cell pipe. Returns 0,
 * or -1 if it can't be done. */
int  rb_pipe_onward(rb_pipe *a, rb_pipe *b);

/* Packs the pipe (see above): `resistance` Pa per m/s of the flow's swings
 * per metre of pipe, on the faces between its cells, for swings faster than
 * `settle_time` (s). */
void rb_pipe_pack(rb_pipe *p, double resistance, double settle_time);

/* Joins end `end` through an orifice, which must join `chamber` and the end's
 * cell (either way round). */
void rb_pipe_attach(rb_pipe *p, int end, rb_chamber *chamber, const rb_orifice *o);

/* Makes the cell at end `end` fast (stepped every physics step). Call after
 * the ends are joined. */
void rb_pipe_set_fast(rb_pipe *p, int end);

/* The cell at an end. */
rb_chamber *rb_pipe_end_cell(rb_pipe *p, int end);

/* One pass of a step (fast: every physics step, dt its length; slow: a
 * block of them, dt the block's):
 *   rb_pipe_clear      clears the pass's cells' rates (before any orifice
 *                      adds its flow to them);
 *   rb_pipe_flows      moves the gas through the pass's faces;
 *   rb_pipe_integrate  integrates the pass's cells, with their own heat:
 *                      the walls', and their gas's kinetic energy;
 *   rb_pipe_accelerate the pass's faces' new flows, from the new pressures
 *                      (once every chamber of the pass has been integrated).
 * A pipe with no fast ends needs only the slow pass, in that order. With two
 * rates (see above): the slow pass's clear and flows when a block begins;
 * the fast pass's clear, flows and integrate every step; the slow pass's
 * integrate and accelerate when the block ends; then the fast accelerate.
 * Slow chambers the pipe joins are stepped with the slow cells. */
void rb_pipe_clear(rb_pipe *p, int fast);
void rb_pipe_flows(rb_pipe *p, int fast, double dt);
void rb_pipe_integrate(rb_pipe *p, int fast, double dt);
void rb_pipe_accelerate(rb_pipe *p, int fast, double dt);

/* What the pipe holds: kg of gas, J of energy (internal and kinetic: what a
 * chamber's books count as U) and kg of each species. NULL skips one. */
void rb_pipe_contents(const rb_pipe *p, double *mass, double *energy,
                      double species[RB_NSPECIES]);

/* The mass flow (kg/s) at a cell's centre, along the pipe: the mean of its
 * two sides' (an end's: its face's or orifice's, 0 when closed). */
double rb_pipe_cell_flow(const rb_pipe *p, int cell);

/* The gas's velocity (m/s) at a cell's centre, along the pipe. */
double rb_pipe_cell_velocity(const rb_pipe *p, int cell);

#endif /* RB_PIPE_H */
