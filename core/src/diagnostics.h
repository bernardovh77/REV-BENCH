#ifndef RB_DIAGNOSTICS_H
#define RB_DIAGNOSTICS_H
/* Optional caller-owned Stage 038 gas-path diagnostics. No allocation or I/O.
 * Slot IDs are (role, number, part), never addresses or engine pipe indices.
 * Transfers: kg, species kg, total transported enthalpy+kinetic J. Momentum
 * entries are staggered-column impulses N s, NOT a fictitious conserved 0D
 * junction/vector momentum. NULL taps preserve the original solver path. */
#include "gas.h"
#include <stddef.h>
#include <stdint.h>

/* Why a solver reports (rb_diag_reason_name gives each one's name). */
enum {
    RB_D_BAD_FLOW,          /* a NaN or infinite flow, set to 0 */
    RB_D_EMPTY,             /* the donor cell was empty */
    RB_D_MASS_CAP,          /* a step would take over half a cell's gas */
    RB_D_SONIC_CAP,         /* the pipes' sonic guard (RB_PIPE_MOST_SONIC) */
    RB_D_DENSITY_FLOOR,
    RB_D_LENGTH_FLOOR,
    RB_D_CHOKED,            /* a nozzle choked: physics, not a guard */
    RB_D_EQUALISE,          /* an orifice's equalisation limit */
    RB_D_DONOR,             /* donor-cell transport */
    RB_D_RECONSTRUCT,       /* limited reconstruction */
    RB_D_MASS_REPAIR,       /* a negative species mass set to 0 */
    RB_D_ENERGY_REPAIR,
    RB_D_TEMP_FLOOR,
    RB_D_PROPERTY_RANGE,    /* T outside the gas fits' range */
    RB_D_NEWTON_FALLBACK,   /* T(u) fell back to the scalar method */
    RB_D_NEWTON_LIMIT,      /* ... or ran out of iterations */
    RB_D_RADIATION_RESET,   /* a non-finite velocity at an outlet's radiation */
    RB_D_HEAT_RESET,        /* a non-finite wall heat, set to 0 */
    RB_D_CFL,               /* a pipe's Courant number above 1 */
    RB_D_REASONS
};
#define RB_D_BIT(reason) (UINT64_C(1) << (reason))
/* A slot's quantities: mass, each species' mass, energy. */
#define RB_D_QUANTITIES (RB_NSPECIES+2)
#define RB_D_MAX_SLOTS 2048
#define RB_D_MAX_TRACE 256
/* What a slot watches ... */
enum { RB_D_VOLUME, RB_D_PIPE, RB_D_CELL, RB_D_FACE, RB_D_ORIFICE };
/* ... and its place in the engine (rb_diag_role_name gives the names). */
enum { RB_D_AMBIENT_IN, RB_D_AMBIENT_OUT, RB_D_AIRBOX, RB_D_PLENUM,
       RB_D_CYLINDER, RB_D_COLLECTOR, RB_D_JUNCTION, RB_D_THROTTLE,
       RB_D_IDLE_AIR, RB_D_INTAKE_VALVE, RB_D_EXHAUST_VALVE,
       RB_D_RUNNER, RB_D_HEADER, RB_D_EXHAUST, RB_D_TAILPIPE,
       RB_D_MUFFLER, RB_D_SNORKEL, RB_D_CENTER, RB_D_LINK };

struct rb_diagnostics;
typedef struct rb_diag_slot {
    struct rb_diagnostics *owner;
    int role, number, kind, part; /* face part -1/-2 = end0/end1 */
    uint64_t reason[RB_D_REASONS], transfers, inventories, invalid;
    /* each quantity's running totals */
    double requested[RB_D_QUANTITIES], accepted[RB_D_QUANTITIES];
    double rejected_abs[RB_D_QUANTITIES], repaired[RB_D_QUANTITIES];
    double impulse_requested, impulse_accepted;
    double max_cfl, length, area, dx, effective_length_max, density_min;
    double inventory[RB_D_QUANTITIES], pressure, temperature;   /* the last inventory */
    int requested_cells, actual_cells;
} rb_diag_slot;

typedef struct rb_diag_record {
    uint64_t tick, reasons;
    size_t slot;
    double requested[RB_D_QUANTITIES], accepted[RB_D_QUANTITIES];
} rb_diag_record;

typedef struct rb_diagnostics {
    rb_diag_slot *slots;
    size_t capacity, count;
    rb_diag_record *trace;
    size_t trace_capacity, trace_count;
    uint64_t tick, generation, dropped, events;
    unsigned trace_stride; /* 0: numerical guard events only; N: every Nth event too */
} rb_diagnostics;

struct rb_engine;
int rb_diag_init(rb_diagnostics *d, rb_diag_slot *slots, size_t capacity,
                 rb_diag_record *trace, size_t trace_capacity, uint64_t generation);
int rb_diag_bind_engine(struct rb_engine *e, rb_diagnostics *d);
const char *rb_diag_reason_name(int reason);
const char *rb_diag_role_name(int role);
void rb_diag_note(rb_diag_slot *s, uint64_t reasons);
void rb_diag_transfer(rb_diag_slot *s, uint64_t reasons, double requested_kg,
                      double accepted_kg, const double y[RB_NSPECIES], double h);
void rb_diag_repair(rb_diag_slot *s, int quantity, double before, double after);
void rb_diag_inventory(rb_diag_slot *s, const double m[RB_NSPECIES], double U, double p, double T);
void rb_diag_impulse(rb_diag_slot *s, uint64_t reasons, double requested, double accepted,
                     double effective_length, double density);
#endif
