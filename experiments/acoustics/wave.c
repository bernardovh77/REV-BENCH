#include "wave.h"
#include <math.h>
#include <string.h>

static int positive(double x) { return isfinite(x) && x > 0.0; }

static int failed(rb_ac_state *s)
{
    s->failures++;
    s->received=0;
    return RB_AC_STATE;
}

int rb_ac_init(rb_ac_plan *p, double rate, double rho, double c)
{
    if (!p || !positive(rate) || rate > 1000000 || rate < 8000 ||
        !positive(rho) || rho > 100 || !positive(c) || c > 3000) return RB_AC_ARGUMENT;
    memset(p, 0, sizeof *p);
    p->rate=rate; p->rho=rho; p->c=c;
    return RB_AC_OK;
}

int rb_ac_add_node(rb_ac_plan *p, int kind, double value)
{
    if (!p || !isfinite(value) || kind < RB_AC_JUNCTION || kind > RB_AC_RADIATION ||
        (kind == RB_AC_JUNCTION && value < 0) ||
        (kind == RB_AC_BOUNDARY && fabs(value) > 1) ||
        (kind == RB_AC_RADIATION && !(value > 0 && value <= 1))) return RB_AC_ARGUMENT;
    if (p->nodes < 0 || p->nodes >= RB_AC_NODES) return RB_AC_CAPACITY;
    const int i=p->nodes++;
    p->node[i]=(rb_ac_node){.kind=kind};
    if (kind == RB_AC_JUNCTION) p->node[i].volume=value;
    if (kind == RB_AC_BOUNDARY) p->node[i].reflection=value;
    if (kind == RB_AC_RADIATION) p->node[i].radius=value;
    return i;
}

int rb_ac_add_edge(rb_ac_plan *p, int a, int b, double length, double area,
                   double gain, int interpolation)
{
    if (!p || a < 0 || b < 0 || a >= p->nodes || b >= p->nodes ||
        !positive(length) || length > 100 || !positive(area) || area > 10 ||
        !isfinite(gain) || gain < 0 || gain > 1 ||
        (interpolation != RB_AC_ALLPASS && interpolation != RB_AC_LINEAR)) return RB_AC_ARGUMENT;
    if (p->edges < 0 || p->edges >= RB_AC_EDGES) return RB_AC_CAPACITY;
    const int i=p->edges++;
    p->edge[i]=(rb_ac_edge){a,b,interpolation,length,area,gain};
    return i;
}

static int validate(const rb_ac_plan *p, size_t *samples)
{
    *samples=0;
    if (!p || !positive(p->rate) || p->rate < 8000 || p->rate > 1000000 ||
        !positive(p->rho) || p->rho > 100 || !positive(p->c) || p->c > 3000 ||
        p->nodes < 1 || p->nodes > RB_AC_NODES || p->edges < 1 ||
        p->edges > RB_AC_EDGES) return RB_AC_ARGUMENT;
    const double rho_c=p->rho*p->c, bulk=rho_c*p->c, dt=1/p->rate;
    if (!positive(rho_c) || !positive(bulk)) return RB_AC_ARGUMENT;
    int degrees[RB_AC_NODES]={0};
    double admittance[RB_AC_NODES]={0};
    for (int i=0;i<p->edges;i++) {
        const rb_ac_edge *e=&p->edge[i];
        if (e->a < 0 || e->a >= p->nodes || e->b < 0 || e->b >= p->nodes ||
            !positive(e->length) || e->length > 100 || !positive(e->area) || e->area > 10 ||
            !isfinite(e->gain) || e->gain < 0 || e->gain > 1 ||
            (e->interpolation != RB_AC_ALLPASS && e->interpolation != RB_AC_LINEAR))
            return RB_AC_ARGUMENT;
        const double d=p->rate*e->length/p->c;
        const double z=rho_c/e->area;
        if (!positive(z) || !positive(1/z) || !positive(dt/z) ||
            !positive(p->rate*z)) return RB_AC_ARGUMENT;
        admittance[e->a]+=1/z; admittance[e->b]+=1/z;
        if (d < 1 || d > 10000000) return RB_AC_SHORT_DELAY;
        *samples += 2*(size_t)floor(d);
        if (++degrees[e->a] > RB_AC_DEGREE || ++degrees[e->b] > RB_AC_DEGREE)
            return RB_AC_CAPACITY;
    }
    for (int n=0;n<p->nodes;n++) {
        const rb_ac_node *v=&p->node[n];
        if (!degrees[n]) return RB_AC_TOPOLOGY;
        if (v->kind == RB_AC_JUNCTION) {
            if (!isfinite(v->volume) || v->volume < 0 || v->volume > 10) return RB_AC_ARGUMENT;
            const double compliance=v->volume/bulk, memory=2*compliance/dt;
            if (!isfinite(compliance) || (v->volume > 0 && !(compliance > 0)) ||
                !isfinite(memory) || !positive(admittance[n]+memory)) return RB_AC_ARGUMENT;
        } else if (v->kind == RB_AC_BOUNDARY) {
            if (degrees[n] != 1 || !isfinite(v->reflection) || fabs(v->reflection) > 1)
                return RB_AC_TOPOLOGY;
        } else if (v->kind == RB_AC_RADIATION) {
            if (degrees[n] != 1 || !positive(v->radius) || v->radius > 1)
                return RB_AC_TOPOLOGY;
            /* Radius specifies this aperture, not an unrelated load diameter. */
            double area=0;
            for (int k=0;k<p->edges;k++)
                if (p->edge[k].a == n || p->edge[k].b == n) area=p->edge[k].area;
            if (fabs(area-3.14159265358979323846*v->radius*v->radius) > 1e-10*area)
                return RB_AC_TOPOLOGY;
            rb_radiation r;
            double direct, history;
            rb_radiation_init(&r,v->radius,p->rho,p->c);
            rb_radiation_linear(&r,dt,&direct,&history);
            if (!positive(r.tau) || !isfinite(direct) || !isfinite(history) ||
                !isfinite(r.b0) || !isfinite(r.b1) || !isfinite(r.b2) ||
                !isfinite(r.a1) || !isfinite(r.a2)) return RB_AC_ARGUMENT;
        } else return RB_AC_TOPOLOGY;
    }
    return RB_AC_OK;
}

size_t rb_ac_samples(const rb_ac_plan *p)
{
    size_t samples;
    return validate(p,&samples) == RB_AC_OK ? samples : 0;
}

int rb_ac_prepare(const rb_ac_plan *p, rb_ac_state *s, double *storage, size_t samples)
{
    size_t need;
    const int status=validate(p,&need);
    if (status) return status;
    if (!s || !storage) return RB_AC_ARGUMENT;
    if (samples < need) return RB_AC_CAPACITY;
    memset(s,0,sizeof *s);
    memset(storage,0,need*sizeof *storage);
    s->plan=p;
    size_t offset=0;
    for (int i=0;i<p->edges;i++) {
        const rb_ac_edge *e=&p->edge[i];
        const double delay=p->rate*e->length/p->c;
        const size_t count=(size_t)floor(delay);
        const double f=delay-count, a=(1-f)/(1+f);
        for (int dir=0;dir<2;dir++) {
            const int port=2*i+dir, node=dir ? e->b:e->a;
            rb_ac_line *l=&s->line[port];
            l->ring=storage+offset; offset+=count;
            l->count=count; l->fraction=f; l->coefficient=a;
            l->complement=sqrt(fmax(0,1-a*a));
            l->interpolation=e->interpolation; l->gain=e->gain;
            l->impedance=s->impedance[port]=p->rho*p->c/e->area;
            s->area[port]=e->area;
            s->ports[node][s->degree[node]++]=port;
        }
    }
    for (int n=0;n<p->nodes;n++)
        if (p->node[n].kind == RB_AC_RADIATION)
            rb_radiation_init(&s->radiation[n],p->node[n].radius,p->rho,p->c);
    return RB_AC_OK;
}

static int valid_drive(const rb_ac_state *s, const rb_ac_drive *drive)
{
    if (!s || !s->plan || !drive) return RB_AC_ARGUMENT;
    if (s->failures) return RB_AC_STATE;
    const rb_ac_plan *p=s->plan;
    for (int n=0;n<p->nodes;n++) {
        const double w=drive->wave[n], q=drive->flow[n];
        if (!isfinite(w) || !isfinite(q) ||
            (p->node[n].kind != RB_AC_BOUNDARY && w != 0) ||
            (p->node[n].kind != RB_AC_JUNCTION && q != 0)) return RB_AC_ARGUMENT;
    }
    return RB_AC_OK;
}

int rb_ac_receive(rb_ac_state *s)
{
    if (!s || !s->plan) return RB_AC_ARGUMENT;
    if (s->failures) return RB_AC_STATE;
    if (s->received) return RB_AC_ORDER;
    const rb_ac_plan *p=s->plan;
    for (int port=0;port<2*p->edges;port++) {
        const rb_ac_line *l=&s->line[port];
        const double x=l->ring[l->index];
        const double y=l->fraction > 0 ?
            (l->interpolation == RB_AC_ALLPASS ? l->coefficient*x+l->complement*l->memory :
             (1-l->fraction)*x+l->fraction*l->memory) : x;
        s->incoming[port^1]=l->gain*y;
        if (!isfinite(s->incoming[port^1])) return failed(s);
    }
    s->received=1;
    return RB_AC_OK;
}

double rb_ac_incident(const rb_ac_state *s, int node)
{
    if (!s || !s->plan || s->failures || !s->received || node < 0 || node >= s->plan->nodes ||
        s->degree[node] != 1) return NAN;
    return s->incoming[s->ports[node][0]];
}

int rb_ac_commit(rb_ac_state *s, const rb_ac_drive *drive)
{
    const int status=valid_drive(s,drive);
    if (status) return status;
    if (!s->received) return RB_AC_ORDER;
    const rb_ac_plan *p=s->plan;
    const double dt=1/p->rate;
    for (int port=0;port<2*p->edges;port++) {
        rb_ac_line *l=&s->line[port];
        const double x=l->ring[l->index];
        double y=x;
        if (l->fraction > 0) {
            if (l->interpolation == RB_AC_ALLPASS) {
                /* Orthogonal two-state realization: x^2+v^2=y^2+v_new^2. */
                y=l->coefficient*x+l->complement*l->memory;
                l->memory=l->complement*x-l->coefficient*l->memory;
            } else {
                y=(1-l->fraction)*x+l->fraction*l->memory;
                s->numerical_loss += dt/l->impedance*l->fraction*(1-l->fraction)
                                      *(x-l->memory)*(x-l->memory);
                l->memory=x;
            }
        }
        s->propagation_loss += dt/l->impedance*(1-l->gain*l->gain)*y*y;
        s->incoming[port^1]=l->gain*y;
    }
    for (int n=0;n<p->nodes;n++) {
        const rb_ac_node *v=&p->node[n];
        const int first=s->ports[n][0];
        double pressure=0;
        if (v->kind == RB_AC_JUNCTION) {
            double admittance=0, incident=0;
            for (int j=0;j<s->degree[n];j++) {
                const int k=s->ports[n][j];
                admittance+=1/s->impedance[k];
                incident+=s->incoming[k]/s->impedance[k];
            }
            const double compliance=v->volume/(p->rho*p->c*p->c);
            const double memory=2*compliance/dt;
            pressure=(2*incident+drive->flow[n]+memory*s->stored_pressure[n])/(admittance+memory);
            if (compliance > 0) s->stored_pressure[n]=2*pressure-s->stored_pressure[n];
            for (int j=0;j<s->degree[n];j++) {
                const int k=s->ports[n][j];
                s->outgoing[k]=pressure-s->incoming[k];
            }
            s->source_work+=dt*pressure*drive->flow[n];
        } else if (v->kind == RB_AC_BOUNDARY) {
            const double a=s->incoming[first], r=v->reflection, w=drive->wave[n];
            const double b=r*a+w;
            s->outgoing[first]=b; pressure=a+b;
            /* The cross term is real source/load interaction, not independent power. */
            s->source_work+=dt/s->impedance[first]*(b*b-r*r*a*a);
            s->terminal_work+=dt/s->impedance[first]*(1-r*r)*a*a;
        } else {
            double direct,history;
            rb_radiation_linear(&s->radiation[n],dt,&direct,&history);
            const double rho_c=s->impedance[first]*s->area[first];
            pressure=(2*direct*s->incoming[first]/rho_c+history)/(1+direct/rho_c);
            const double b=pressure-s->incoming[first];
            s->outgoing[first]=b;
            const double velocity=(s->incoming[first]-b)/rho_c;
            /* Do not silently invoke the shared filter's protective reset. */
            if (!isfinite(velocity) || fabs(velocity) > 1e6 ||
                !isfinite(rb_radiation_step(&s->radiation[n],velocity))) return failed(s);
            s->terminal_work+=dt/s->impedance[first]
                *(s->incoming[first]*s->incoming[first]-b*b);
        }
        s->pressure[n]=pressure;
        if (!isfinite(pressure) || !isfinite(s->stored_pressure[n]) ||
            !isfinite(s->source_work) || !isfinite(s->terminal_work)) return failed(s);
    }
    for (int port=0;port<2*p->edges;port++) {
        rb_ac_line *l=&s->line[port];
        const double b=s->outgoing[port], old=l->ring[l->index];
        if (!isfinite(b)) return failed(s);
        l->energy += b*b-old*old;
        if (!isfinite(l->energy)) return failed(s);
        l->ring[l->index]=b;
        l->index=(l->index+1)%l->count;
    }
    const double energy=rb_ac_energy(s);
    const double balance=s->source_work-s->terminal_work-s->numerical_loss-s->propagation_loss-energy;
    if (!isfinite(s->numerical_loss) || !isfinite(s->propagation_loss) ||
        !isfinite(energy) || !isfinite(balance)) return failed(s);
    s->steps++;
    s->received=0;
    return RB_AC_OK;
}

int rb_ac_step(rb_ac_state *s, const rb_ac_drive *drive)
{
    const int status=valid_drive(s,drive);
    if (status) return status;
    const int received=rb_ac_receive(s);
    return received ? received : rb_ac_commit(s,drive);
}

double rb_ac_energy(const rb_ac_state *s)
{
    if (!s || !s->plan || s->failures) return NAN;
    const rb_ac_plan *p=s->plan;
    double energy=0;
    for (int k=0;k<2*p->edges;k++) {
        const rb_ac_line *l=&s->line[k];
        const double weight=l->interpolation == RB_AC_ALLPASS ? 1:l->fraction;
        energy+=(l->energy+weight*l->memory*l->memory)/(p->rate*l->impedance);
    }
    for (int n=0;n<p->nodes;n++) {
        const double pressure=s->stored_pressure[n];
        energy+=.5*p->node[n].volume/(p->rho*p->c*p->c)*pressure*pressure;
    }
    return energy;
}

double rb_ac_balance(const rb_ac_state *s)
{
    if (!s || !s->plan || s->failures) return NAN;
    return s->source_work-s->terminal_work-s->numerical_loss-s->propagation_loss-rb_ac_energy(s);
}

double rb_ac_node_flow(const rb_ac_state *s, int node)
{
    if (!s || !s->plan || s->failures || node < 0 || node >= s->plan->nodes || s->degree[node] != 1) return NAN;
    const int k=s->ports[node][0];
    return (s->incoming[k]-s->outgoing[k])/s->impedance[k];
}
