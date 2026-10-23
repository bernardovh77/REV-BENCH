#ifndef RB_COUPLED_DUCT_H
#define RB_COUPLED_DUCT_H

/* Native homogeneous constant-gamma open-port proof. ONE physical conserved
 * state per cell; downstream characteristics plus spatial nonlinear/entropy
 * corrections, not a duplicate gas/acoustic energy reservoir. No engine ABI.
 * Only sample-aligned uniform ducts, initially quiescent background, supported.
 * Workspace and bounded plans are caller-owned; no allocation or silent repair.
 */
#include <stddef.h>
typedef struct { double v[4]; } rb_cd_vec; /* rho, rho*u, total J/m3, tagged kg/m3 */
typedef struct {
    int cells, gas_cells;
    double rate, length, area, rho, pressure, gamma, tag_fraction, reflection;
} rb_cd_spec;
typedef struct {
    rb_cd_spec spec;
    double c, dx, h0, e0;
    rb_cd_vec *q, *waves, *gas, *stage, *rhs, *slope, *face;
    rb_cd_vec initial[2], external_left, external_right, interface_transfer;
    /* Integrated transfer gas->network: kg, N*s, J (already includes pressure
     * work), tagged kg. Never add a second acoustic work to this energy. */
    rb_cd_vec last_interface;
    /* Linearized conserved-state characteristics: a+b=(gamma-1)*(E-E0),
     * NOT physical p-p0 at finite velocity; kinetic pressure is subtracted. */
    double current_return_Pa, accepted_outgoing_Pa, max_pressure_fraction;
    double max_residual_cfl;
    unsigned long long steps, failures;
    int failed_stage;
} rb_cd_state;
enum { RB_CD_OK=0, RB_CD_ARGUMENT=-1, RB_CD_CAPACITY=-2,
       RB_CD_GRID=-3, RB_CD_STATE=-4 };
size_t rb_cd_workspace(int cells); /* count of rb_cd_vec, not bytes */
int rb_cd_prepare(rb_cd_state *s, const rb_cd_spec *p, rb_cd_vec *memory, size_t count);
int rb_cd_seed(rb_cd_state *s, double amplitude, double center, double width);
int rb_cd_load(rb_cd_state *s, const rb_cd_vec *initial, int cells);
int rb_cd_step(rb_cd_state *s);
double rb_cd_pressure(const rb_cd_state *s, int cell); /* actual physical Pa */
rb_cd_vec rb_cd_balance(const rb_cd_state *s, int side); /* total - initial - accepted */
#endif
