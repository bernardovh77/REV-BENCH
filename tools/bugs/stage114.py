"""114 physical current-face/enthalpy/contact and fixed-inventory faults."""
BUGS={
 'exterior-loss-per-cell':('experiments/acoustics/fixed_transport.c','exp(-s->outer*dt/s->wall_capacity)','exp(-s->cells*s->outer*dt/s->wall_capacity)',['test_fixed_transport_separate_fronts_and_fixed_exterior'],180),
 'unchecked-final-heat':('experiments/acoustics/fixed_transport.c','if(p->wall_capacity&&rb_ft_decode(p,p->slot[i].q,&p->slot[i].w))return fail(p);','/* faulty final heat state left unchecked */',['test_fixed_transport_reversal_wall_and_domain'],180),
 'wrong-interface-impedance':('experiments/acoustics/fixed_transport.c','double sl=a.u-a.c,sr=b.u+b.c;','double sl=fmin(a.u-a.c,b.u-b.c),sr=fmax(a.u+a.c,b.u+b.c);',['test_fixed_transport_current_shared_port'],180),
 'cancelled-contact-mass':('experiments/acoustics/fixed_transport.c','rb_ft_q flux={{star.r[0]*sm,star.r[1]*sm}','rb_ft_q flux={{fl.r[0]+sl*(star.r[0]-l.r[0]),fl.r[1]+sl*(star.r[1]-l.r[1])}',['test_fixed_transport_contact_and_stationary_books'],180),
 'double-pressure-enthalpy':('experiments/acoustics/fixed_transport.c','(star.energy+ps)*sm','(star.energy+2*ps)*sm',['test_fixed_transport_reversal_wall_and_domain'],180),
 'stale-shared-interface':('experiments/acoustics/fixed_transport.c','rb_ft_riemann(a,reconstruct(a,a->cells-1,.5),reconstruct(b,0,-.5),&shared)','rb_ft_riemann(a,a->slot[a->cells-1].q,b->slot[0].q,&shared)',['test_fixed_transport_current_shared_port'],180),
 'different-shared-energy':('experiments/acoustics/fixed_transport.c','a->slot[a->cells].face=b->slot[0].face=shared;','a->slot[a->cells].face=b->slot[0].face=shared;b->slot[0].face.f.energy*=2;',['test_fixed_transport_current_shared_port'],180),
 'dropped-material-book':('experiments/acoustics/fixed_transport.c','b->m[k]+=factor*f.r[k];','if(k==0)b->m[k]+=factor*f.r[k];',['test_fixed_transport_contact_and_stationary_books'],180),
 'stale-rk-transfer-book':('experiments/acoustics/fixed_transport.c','add(p->slot[i].saved.f,1,p->slot[i].face.f)','add(p->slot[i].saved.f,1,p->slot[i].saved.f)',['test_fixed_transport_current_shared_port'],180),
 'unbooked-wall-energy':('experiments/acoustics/fixed_transport.c','s->slot[i].q.energy+=Q/V;','s->slot[i].q.energy+=0*Q/V;',['test_fixed_transport_reversal_wall_and_domain'],180),
 'reused-failed-state':('experiments/acoustics/fixed_transport.c','if(a->failed||(b&&b->failed))return -2;','/* faulty failed-state reuse */',['test_fixed_transport_reversal_wall_and_domain'],180),
 'unloaded-step':('experiments/acoustics/fixed_transport.c','!a->slot||!a->loaded||!positive(dt)','!a->slot||!positive(dt)',['test_fixed_transport_reversal_wall_and_domain'],180),
 'pressure-damping-overwrite':('experiments/acoustics/fixed_transport.c','if(!positive(p)||fabs(p/s->p0-1)>.03)','p=s->p0+.5*(p-s->p0);if(!positive(p)||fabs(p/s->p0-1)>.03)',['test_fixed_transport_current_shared_port'],180),
}
