#include "diagnostics.h"
#include "engine.h"
#include <math.h>
#include <string.h>

const char *rb_diag_reason_name(int r)
{
    static const char *const names[]={"invalid-flow-reset","empty-donor","transport-mass-cap",
        "pipe-sonic-cap","density-floor","momentum-length-floor","physical-nozzle-choke",
        "orifice-equalisation-cap","donor-reconstruction","limited-reconstruction",
        "species-repair","energy-repair","temperature-floor","property-extrapolation",
        "scalar-fallback","newton-iteration-limit","radiation-velocity-reset",
        "invalid-wall-heat-reset","runtime-cfl-above-one"};
    return r>=0 && r<RB_D_REASONS ? names[r]:"unknown";
}
const char *rb_diag_role_name(int r)
{
    static const char *const names[]={"ambient-intake","ambient-exhaust","airbox","plenum",
        "cylinder","collector","junction","throttle","idle-air","intake-valve","exhaust-valve",
        "runner","header","exhaust","tailpipe","muffler","snorkel","center","link"};
    return r>=0 && r<(int)(sizeof names/sizeof names[0]) ? names[r]:"unknown";
}
int rb_diag_init(rb_diagnostics *d,rb_diag_slot *slots,size_t cap,
                 rb_diag_record *trace,size_t n,uint64_t generation)
{
    if(!d || !slots || cap<1 || cap>RB_D_MAX_SLOTS || n>RB_D_MAX_TRACE || (n && !trace)) return -1;
    memset(d,0,sizeof *d);
    d->slots=slots;d->capacity=cap;d->trace=trace;d->trace_capacity=n;d->generation=generation;
    return 0;
}
static void record(rb_diag_slot *s,uint64_t reasons,const double *request,const double *accept)
{
    rb_diagnostics *d=s->owner;
    const uint64_t ordinary=RB_D_BIT(RB_D_CHOKED)|RB_D_BIT(RB_D_DONOR)|RB_D_BIT(RB_D_RECONSTRUCT);
    d->events++;
    if(!d->trace_capacity || (!(reasons & ~ordinary) &&
       (!d->trace_stride || d->events%d->trace_stride))) return;
    if(d->trace_count==d->trace_capacity) {d->dropped++;return;}
    rb_diag_record *r=&d->trace[d->trace_count++];
    memset(r,0,sizeof *r);r->tick=d->tick;r->slot=(size_t)(s-d->slots);r->reasons=reasons;
    if(request) memcpy(r->requested,request,sizeof r->requested);
    if(accept) memcpy(r->accepted,accept,sizeof r->accepted);
}
static void reasons_add(rb_diag_slot *s,uint64_t reasons)
{
    if(!reasons) return;
    for(int i=0;i<RB_D_REASONS;i++) if(reasons & RB_D_BIT(i)) s->reason[i]++;
}
void rb_diag_note(rb_diag_slot *s,uint64_t reasons)
{
    if(!s) return;
    reasons_add(s,reasons);record(s,reasons,NULL,NULL);
}
static void add(rb_diag_slot *s,double *total,double value)
{
    if(!isfinite(value) || !isfinite(*total+value)) s->invalid++;
    else *total+=value;
}
void rb_diag_transfer(rb_diag_slot *s,uint64_t reasons,double request,double accept,
                      const double y[RB_NSPECIES],double h)
{
    if(!s) return;
    double q[RB_D_QUANTITIES],a[RB_D_QUANTITIES];
    q[0]=request;a[0]=accept;
    for(int j=0;j<RB_NSPECIES;j++) {q[j+1]=request*y[j];a[j+1]=accept*y[j];}
    q[RB_D_QUANTITIES-1]=request*h;a[RB_D_QUANTITIES-1]=accept*h;
    for(int j=0;j<RB_D_QUANTITIES;j++) {
        add(s,&s->requested[j],q[j]);add(s,&s->accepted[j],a[j]);
        add(s,&s->rejected_abs[j],fabs(q[j]-a[j]));
        if(!isfinite(q[j])) q[j]=0;
        if(!isfinite(a[j])) a[j]=0;
    }
    s->transfers++;reasons_add(s,reasons);record(s,reasons,q,a);
}
void rb_diag_repair(rb_diag_slot *s,int quantity,double before,double after)
{
    if(!s) return;
    if(quantity>=0 && quantity<RB_D_QUANTITIES) add(s,&s->repaired[quantity],after-before);
    if(quantity>0 && quantity<RB_D_QUANTITIES-1) add(s,&s->repaired[0],after-before);
    rb_diag_note(s,RB_D_BIT(quantity==RB_D_QUANTITIES-1 ? RB_D_ENERGY_REPAIR:RB_D_MASS_REPAIR));
}
void rb_diag_inventory(rb_diag_slot *s,const double m[RB_NSPECIES],double U,double p,double T)
{
    if(!s) return;
    s->inventories++;s->inventory[0]=0;
    for(int j=0;j<RB_NSPECIES;j++) {
        s->inventory[j+1]=m[j];s->inventory[0]+=m[j];
        if(!isfinite(m[j])) s->invalid++;
    }
    s->inventory[RB_D_QUANTITIES-1]=U;s->pressure=p;s->temperature=T;
    if(T<RB_T_LO || T>RB_T_HI) rb_diag_note(s,RB_D_BIT(RB_D_PROPERTY_RANGE));
    if(!isfinite(p) || !isfinite(T) || !isfinite(U)) s->invalid++;
}
void rb_diag_impulse(rb_diag_slot *s,uint64_t reasons,double request,double accept,double length,double rho)
{
    if(!s) return;
    add(s,&s->impulse_requested,request);add(s,&s->impulse_accepted,accept);
    if(length>s->effective_length_max) s->effective_length_max=length;
    if(!s->density_min || rho<s->density_min) s->density_min=rho;
    rb_diag_note(s,reasons);
}
static rb_diag_slot *slot(rb_diagnostics *d,size_t *n,int role,int number,int kind,int part)
{
    if(!d) return NULL;
    rb_diag_slot *s=&d->slots[(*n)++];memset(s,0,sizeof *s);
    s->owner=d;s->role=role;s->number=number;s->kind=kind;s->part=part;
    return s;
}
static int pipe_role(const rb_engine *e,const rb_pipe *p,int *index)
{
    for(int i=0;i<e->desc.cylinders;i++) {
        if(p==&e->runner[i]) {*index=i;return RB_D_RUNNER;}
        if(p==&e->header[i]) {*index=i;return RB_D_HEADER;}
    }
    for(int i=0;i<e->collectors;i++) {
        if(p==&e->exhaust_pipe[i]) {*index=i;return RB_D_EXHAUST;}
        if(p==&e->center[i]) {*index=i;return RB_D_CENTER;}
        if(p==&e->link[i]) {*index=i;return RB_D_LINK;}
    }
    for(int i=0;i<e->systems;i++) {
        if(p==&e->tailpipe[i]) {*index=i;return RB_D_TAILPIPE;}
        if(p==&e->muffler[i]) {*index=i;return RB_D_MUFFLER;}
    }
    *index=0;return p==&e->snorkel ? RB_D_SNORKEL:-1;
}
int rb_diag_bind_engine(rb_engine *e,rb_diagnostics *d)
{
    if(!e) return -1;
    size_t need=(size_t)(6+3*e->desc.cylinders+e->collectors+(e->desc.junction_volume>0));
    for(int k=0;k<rb_engine_pipe_count(e);k++) need+=(size_t)(2*rb_engine_pipe(e,k)->cells+2);
    if(d && (!d->slots || need>d->capacity)) return -1;
    size_t n=0;
    e->diag=d;
#define BIND(object,role,num,kind,part) (object).diag=slot(d,&n,role,num,kind,part)
    BIND(e->intake_air,RB_D_AMBIENT_IN,0,RB_D_VOLUME,0);
    BIND(e->exhaust_air,RB_D_AMBIENT_OUT,0,RB_D_VOLUME,0);
    BIND(e->airbox,RB_D_AIRBOX,0,RB_D_VOLUME,0);BIND(e->plenum,RB_D_PLENUM,0,RB_D_VOLUME,0);
    BIND(e->throttle,RB_D_THROTTLE,0,RB_D_ORIFICE,0);BIND(e->idle_air,RB_D_IDLE_AIR,0,RB_D_ORIFICE,0);
    for(int i=0;i<e->desc.cylinders;i++) {
        BIND(e->cyl[i].gas,RB_D_CYLINDER,i,RB_D_VOLUME,0);
        BIND(e->intake[i],RB_D_INTAKE_VALVE,i,RB_D_ORIFICE,0);
        BIND(e->exhaust[i],RB_D_EXHAUST_VALVE,i,RB_D_ORIFICE,0);
    }
    for(int i=0;i<e->collectors;i++) BIND(e->collector[i],RB_D_COLLECTOR,i,RB_D_VOLUME,0);
    if(e->desc.junction_volume>0) BIND(e->junction,RB_D_JUNCTION,0,RB_D_VOLUME,0);
    for(int k=0;k<rb_engine_pipe_count(e);k++) {
        rb_pipe *p=(rb_pipe *)rb_engine_pipe(e,k);int number;
        const int role=pipe_role(e,p,&number);
        if(role<0) return -1;
        BIND(*p,role,number,RB_D_PIPE,0);
        if(p->diag) {
            p->diag->length=p->length;p->diag->area=p->area;p->diag->dx=p->dx;
            p->diag->requested_cells=p->requested_cells;p->diag->actual_cells=p->cells;
        }
        for(int j=0;j<p->cells;j++) BIND(p->cell[j],role,number,RB_D_CELL,j);
        for(int j=0;j<p->cells-1;j++) BIND(p->face[j],role,number,RB_D_FACE,j);
        for(int end=0;end<2;end++) {
            BIND(p->end_face[end],role,number,RB_D_FACE,-1-end);
            p->radiation[end].diag=p->end_face[end].diag;
        }
    }
#undef BIND
    if(d) {d->count=n;d->tick=0;d->trace_count=0;d->dropped=0;d->events=0;}
    return 0;
}
