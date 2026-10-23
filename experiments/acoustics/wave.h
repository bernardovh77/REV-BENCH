#ifndef RB_EXPERIMENT_WAVE_H
#define RB_EXPERIMENT_WAVE_H

/* Native S2d-B0 research implementation. Frozen homogeneous gas, U=0;
 * pressure waves in Pa, volume flow in m^3/s, power in W. No engine adapter.
 * Plans are immutable after prepare; storage is caller-owned. Derived
 * coefficients must be finite/representable. These guards do not qualify
 * arbitrary amplitudes or extend the frozen, linear acoustic model. */
#include "radiation.h"
#include <stddef.h>

#define RB_AC_NODES 32
#define RB_AC_EDGES 48
#define RB_AC_PORTS (2 * RB_AC_EDGES)
#define RB_AC_DEGREE 8

enum { RB_AC_JUNCTION, RB_AC_BOUNDARY, RB_AC_RADIATION };
enum { RB_AC_ALLPASS, RB_AC_LINEAR };
enum { RB_AC_OK = 0, RB_AC_ARGUMENT = -1, RB_AC_CAPACITY = -2,
       RB_AC_TOPOLOGY = -3, RB_AC_SHORT_DELAY = -4, RB_AC_STATE = -5,
       RB_AC_ORDER = -6 };

typedef struct {
    int kind;
    double volume, reflection, radius;
} rb_ac_node;
typedef struct {
    int a, b, interpolation;
    double length, area, gain;
} rb_ac_edge;
typedef struct {
    double rate, rho, c;
    int nodes, edges;
    rb_ac_node node[RB_AC_NODES];
    rb_ac_edge edge[RB_AC_EDGES];
} rb_ac_plan;
typedef struct {
    double *ring;
    size_t count, index;
    double fraction, coefficient, complement, memory, energy, impedance, gain;
    int interpolation;
} rb_ac_line;
typedef struct {
    const rb_ac_plan *plan;
    rb_ac_line line[RB_AC_PORTS];
    int degree[RB_AC_NODES], ports[RB_AC_NODES][RB_AC_DEGREE];
    double impedance[RB_AC_PORTS], area[RB_AC_PORTS];
    double incoming[RB_AC_PORTS], outgoing[RB_AC_PORTS];
    double pressure[RB_AC_NODES], stored_pressure[RB_AC_NODES];
    rb_radiation radiation[RB_AC_NODES];
    /* terminal_work includes radiation-filter storage, not just radiation loss.
     * numerical_loss is interpolation loss; propagation_loss is specified gain. */
    double source_work, terminal_work, numerical_loss, propagation_loss;
    unsigned long long steps, failures;
    int received; /* current tick's incident waves exposed; storage not consumed */
} rb_ac_state;
typedef struct {
    double wave[RB_AC_NODES]; /* incident drive Pa at one-port boundaries */
    double flow[RB_AC_NODES]; /* injected m^3/s at junctions/cavities */
} rb_ac_drive;

int rb_ac_init(rb_ac_plan *p, double rate, double rho, double c);
int rb_ac_add_node(rb_ac_plan *p, int kind, double value);
int rb_ac_add_edge(rb_ac_plan *p, int a, int b, double length, double area,
                   double gain, int interpolation);
/* Required double samples, or zero for invalid/unsupported plans. */
size_t rb_ac_samples(const rb_ac_plan *p);
int rb_ac_prepare(const rb_ac_plan *p, rb_ac_state *s, double *storage, size_t samples);
int rb_ac_step(rb_ac_state *s, const rb_ac_drive *drive);
/* Receive is read-only with respect to delay/storage/work. Commit accepts this
 * tick's drive once; invalid drives can be corrected and retried without delay.
 * A numerical RB_AC_STATE is terminal: work/storage may be partly consumed;
 * all processing rejects until prepare resets the state. Observers return NaN.
 * Port flow positive INTO graph at a source is -rb_ac_node_flow(s,node). */
int rb_ac_receive(rb_ac_state *s);
double rb_ac_incident(const rb_ac_state *s, int node); /* one-port, Pa */
int rb_ac_commit(rb_ac_state *s, const rb_ac_drive *drive);
double rb_ac_energy(const rb_ac_state *s);
double rb_ac_balance(const rb_ac_state *s); /* starts from rest */
double rb_ac_node_flow(const rb_ac_state *s, int node); /* into one-port node */

#endif
