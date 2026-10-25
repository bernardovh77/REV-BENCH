#ifndef RB_EXPERIMENT_GAS_GRAPH_H
#define RB_EXPERIMENT_GAS_GRAPH_H
/* 045: one uniform duct, production-EOS retained gas + actual B0 delay graph.
 * Upstream q, downstream B0 acoustic coordinates and contact coordinates own
 * ONE conserved physical state. The downstream q cache is derived only.
 * No engine ABI/renderer promotion. Caller-owned immutable plan/workspace.
 * Non-sample-aligned lengths supported; junctions/heat/losses not qualified.
 * One failed numerical step poisons the object; prepare is required to retry. */
#include "mixture.h"
#include "wave.h"
typedef struct {
    double rate,length,interface_x,area,pressure,temperature,y[RB_NSPECIES],reflection;
    int gas_cells;
} rb_gg_spec;
typedef struct {
    rb_gg_spec spec;
    rb_ac_plan plan;rb_ac_state graph;
    rb_mix_cell base;
    double rho,c,beta,h0,coefficient[RB_NSPECIES],dx,gas_dx,fraction;
    int downstream_cells,cells;
    rb_mix_cell *q,*w,*work,*original,*rhs,*contact,*slope,*face;
    double *ring_copy,memory_copy[2];
    rb_mix_cell initial[2],external_left,external_right,interface_transfer;
    rb_mix_cell last_interface,last_linear,last_residual;
    double received_return,used_return,accepted_outgoing,previous_return;
    double coordinate_work,max_residual_cfl,max_pressure_fraction;
    unsigned long long steps,failures,domain_rejections,linear_smooth,linear_limited,residual_fallbacks;
} rb_gg_state;
enum {RB_GG_OK=0,RB_GG_ARGUMENT=-1,RB_GG_CAPACITY=-2,RB_GG_DOMAIN=-3,RB_GG_STATE=-4};
int rb_gg_sizes(const rb_gg_spec *p,size_t *cells,size_t *doubles);
int rb_gg_prepare(rb_gg_state *s,const rb_gg_spec *p,rb_mix_cell *cells,size_t count,
                  double *delays,size_t samples);
/* Initial downstream loading is stationary contact-only; acoustic pulses
 * originate in retained upstream gas, not independently prescribed B0 drives. */
int rb_gg_load(rb_gg_state *s,const rb_mix_cell *q,int cells);
int rb_gg_seed(rb_gg_state *s,double amplitude,double center,double width,double frequency);
int rb_gg_step(rb_gg_state *s);
rb_mix_cell rb_gg_balance(rb_gg_state *s,int side);
rb_mix_cell rb_gg_totals(rb_gg_state *s,int side);
double rb_gg_pressure(rb_gg_state *s,int cell);
double rb_gg_perturbation_energy(rb_gg_state *s);
double rb_gg_cell_width(const rb_gg_state *s,int cell);
double rb_gg_cell_center(const rb_gg_state *s,int cell);
#endif
