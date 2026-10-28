#ifndef RB_EXPERIMENT_JUNCTION_H
#define RB_EXPERIMENT_JUNCTION_H
/* 049 stationary finite-volume vector-Euler junction reference. Geometry is
 * supplied once, SI, convex cells. A boundary face is either an impermeable
 * wall or a current physical port. No runtime engine or real-loss promotion. */
#include "fixed_transport.h"
#define RB_JV_PORTS 16
#define RB_JV_REGIONS 32
#define RB_JV_DEGREE 8
typedef struct {double r[2],momentum[3],energy,alpha[2];} rb_jv_q;
typedef struct {double volume,center[3];int region;} rb_jv_cell_geometry;
typedef struct {int a,b,port;double area,normal[3],center[3];} rb_jv_face_geometry;
typedef struct {
    rb_jv_q q,stage,next,rhs;
    double w[8],gradient[8][3],inverse_metric[3][3];
    int degree,face[RB_JV_DEGREE],sign[RB_JV_DEGREE],component;
} rb_jv_cell;
typedef struct {rb_jv_q flux,saved;double u,p,saved_u,saved_p;} rb_jv_face;
typedef struct {
    int cells,faces,ports,loaded,failed,components;
    const rb_jv_cell_geometry *geometry;
    const rb_jv_face_geometry *connection;
    rb_jv_cell *cell;rb_jv_face *face;
    rb_ft_state thermo;
    rb_jv_q initial,external,regional_initial[RB_JV_REGIONS],regional_transfer[RB_JV_REGIONS];
    double port_area[RB_JV_PORTS],port_normal[RB_JV_PORTS][3],port_plane[RB_JV_PORTS];
    int port_component[RB_JV_PORTS];
    double port_pressure[RB_JV_PORTS],port_velocity[RB_JV_PORTS];
    rb_jv_q port_transfer[RB_JV_PORTS];double wall_impulse[3];
    double max_cfl,max_mach,max_pressure_fraction;
    unsigned long long steps,reconstruction_fallbacks,trace_face_limits;
} rb_jv_state;
int rb_jv_prepare(rb_jv_state *s,int cells,int faces,int ports,
                   const rb_jv_cell_geometry *geometry,const rb_jv_face_geometry *connection,
                   rb_jv_cell *storage,size_t count,rb_jv_face *face_storage,size_t face_count,
                   double p0,const double y0[3],const double y1[3],double T0,double T1);
rb_jv_q rb_jv_make(const rb_jv_state *s,double p,const double velocity[3],
                    const double alpha[2],const double T[2]);
int rb_jv_decode(const rb_jv_state *s,rb_jv_q q,double w[8],double *rho,double *c);
int rb_jv_load(rb_jv_state *s);
int rb_jv_flux(const rb_jv_state *s,rb_jv_q a,rb_jv_q b,const double n[3],rb_jv_q *F,double *u,double *p);
int rb_jv_step(rb_jv_state *s,double dt,const rb_jv_q *boundary_start,
                 const rb_jv_q *boundary_end,size_t count);
rb_jv_q rb_jv_inventory(const rb_jv_state *s,int region);
double rb_jv_perturbation(const rb_jv_state *s,const double U[3]);
#endif
