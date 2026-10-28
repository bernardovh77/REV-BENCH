"""049 vector, stationary-junction, port and trace-material faults."""
F='experiments/acoustics/junction.c'
BUGS={
 'excluded-near-unit-average':(F,'upper=fmin(upper,1+1e-12);','upper=fmin(upper,1);', ['test_junction_trace_material_at_one_sided_open_port'],180),
 'recreated-subnormal-mass':(F,'for(int k=0;k<2;k++)if(trace[k])q.r[k]=current.r[k];','/* faulty trace mass rebuilt from alpha */', ['test_junction_trace_material_at_one_sided_open_port'],180),
 'missing-transverse-energy':(F,'f.f.energy+.5*mass*dot(vt,vt)','f.f.energy', ['test_junction_vector_flux_and_cached_thermodynamics'],180),
 'missing-transverse-momentum':(F,'f.f.momentum*n[j]+mass*vt[j]','f.f.momentum*n[j]', ['test_junction_vector_flux_and_cached_thermodynamics'],180),
 'opposite-face-sign':(F,'double factor=(side?1:-1)*g->area/s->geometry[i].volume','double factor=-g->area/s->geometry[i].volume', ['test_junction_regional_accounts_and_domain'],180),
 'wall-energy-leak':(F,'memset(&f->flux,0,sizeof f->flux);f->u=0;','memset(&f->flux,0,sizeof f->flux);f->flux.energy=1;f->u=0;', ['test_junction_current_rk_wall_and_closed_energy'],180),
 'shared-ambient-shortcut':(F,'r=boundary[g->port];','r=boundary[0];', ['test_junction_disconnected_ports_do_not_share_ambient'],180),
 'stale-rk-cell':(F,'rb_jv_q q=stage?c->stage:c->q;if(rb_jv_decode','rb_jv_q q=c->q;if(rb_jv_decode', ['test_junction_current_rk_wall_and_closed_energy'],180),
 'stale-accepted-transfer':(F,'add(f->saved,1,f->flux)','add(f->saved,1,f->saved)', ['test_junction_current_ports_and_reversal'],180),
 'lost-regional-energy':(F,'s->regional_transfer[a]=add(s->regional_transfer[a],-1,accepted);','s->regional_transfer[a]=add(s->regional_transfer[a],-1,accepted);s->regional_transfer[a].energy=0;', ['test_junction_regional_accounts_and_domain'],180),
 'unchecked-volume':(F,'||fabs(V/g[i].volume-1)>1e-10','', ['test_junction_regional_accounts_and_domain'],180),
 'unchecked-clock':(F,'return cfl>.45?failure(s):0;','return 0;', ['test_junction_current_rk_wall_and_closed_energy'],180),
 'failed-state-reuse':(F,'if(s->failed)return -2;','/* faulty reuse */', ['test_junction_current_rk_wall_and_closed_energy'],180),
 'unbounded-open-fraction':(F,'if(v>=A0){lower=fmax(lower,0);upper=fmin(upper,1+1e-12);}','/* faulty open fraction extrapolation */', ['test_junction_trace_material_at_one_sided_open_port'],180),
 'underflow-ratio-query':(F,'normal.r[k]=0;normal.alpha[k]=0;','/* faulty subnormal ratio passed to114 EOS */', ['test_junction_trace_material_at_one_sided_open_port'],180),
 'pressure-overwrite':(F,'w[P]=a.p;for(int j=0;j<3;j++)','w[P]=s->thermo.p0+.5*(a.p-s->thermo.p0);for(int j=0;j<3;j++)', ['test_junction_vector_flux_and_cached_thermodynamics'],180),
}
