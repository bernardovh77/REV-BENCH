"""048 material/contact-compatible force, work and fixed-wall faults."""
BUGS={
 'position-subtraction-work':('experiments/acoustics/material.c','const double deltaV=.5*dt*s->area*((s->node[i+1].v-s->node[i].v)+(s->node[i+1].next_v-s->node[i].next_v));','const double deltaV=s->area*((s->node[i+1].next_x-s->node[i].next_x)-(s->node[i+1].x-s->node[i].x));',['test_material_fine_geometry_work'],180),
 'unloaded-state-acceptance':('experiments/acoustics/material.c','!s->cell||!s->loaded||!positive(dt)','!s->cell||!positive(dt)',['test_material_coordinate_frame_and_failure'],180),
 'stale-pressure-force':('experiments/acoustics/material.c','s->cell[i-1].midpoint-s->cell[i].midpoint','s->cell[i-1].p-s->cell[i].p',['test_material_pulse_force_and_current_work'],180),
 'duplicate-pressure-work':('experiments/acoustics/material.c','c->next_E=c->E-c->midpoint*deltaV;','c->next_E=c->E-2*c->midpoint*deltaV;',['test_material_pulse_force_and_current_work'],180),
 'dropped-fuel-inventory':('experiments/acoustics/material.c','species[j]+=f*s->cell[i].m[j];','if(j!=1)species[j]+=f*s->cell[i].m[j];',['test_material_hot_contact_and_translation'],180),
 'missing-body-work':('experiments/acoustics/material.c','s->body_work+=dt*node->mass','s->body_work+=0*dt*node->mass',['test_material_reversal_and_fixed_wall'],180),
 'unbooked-temperature-rewrite':('experiments/acoustics/material.c','c->p=c->mass*c->gas.R*c->T/V;','c->T-=.001;c->E=c->mass*rb_gas_u(&c->gas,c->T);c->p=c->mass*c->gas.R*c->T/V;',['test_material_hot_contact_and_translation'],180),
 'moving-metal':('experiments/acoustics/material.c','const double lo=s->node[i].x+s->shift,hi=s->node[i+1].x+s->shift;','const double lo=s->node[i].x,hi=s->node[i+1].x;',['test_material_reversal_and_fixed_wall'],180),
 'exterior-loss-per-gas-cell':('experiments/acoustics/material.c','exp(-s->outer*dt/s->wall_capacity)','exp(-s->cells*s->outer*dt/s->wall_capacity)',['test_material_reversal_and_fixed_wall'],180),
 'coordinate-frame-double-drift':('experiments/acoustics/material.c','.5*(node->v+node->next_v)-s->frame_U','.5*(node->v+node->next_v)',['test_material_coordinate_frame_and_failure'],180),
 'reused-failed-state':('experiments/acoustics/material.c','if(s->failed)return -2;','/* faulty retry after failure */',['test_material_coordinate_frame_and_failure'],180),
}
