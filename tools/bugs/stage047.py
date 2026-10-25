"""047 gas/wall energy, geometry and thermodynamic identity faults."""
BUGS={
 'wrong-wall-energy-sign':('experiments/acoustics/thermal.c','C*s->wall_T-Q+dt*s->outer*s->ambient_T','C*s->wall_T+Q+dt*s->outer*s->ambient_T',['test_thermal_capacity_and_closed_books'],180),
 'missing-exterior-book':('experiments/acoustics/thermal.c','s->ambient_heat+=dt*s->outer','s->ambient_heat+=0*dt*s->outer',['test_thermal_capacity_and_closed_books'],180),
 'duplicated-wall-capacity':('experiments/acoustics/thermal.c','3.14159265358979323846*length*thickness','3.14159265358979323846*thickness',['test_thermal_capacity_and_closed_books'],180),
 'lost-gas-heat':('experiments/acoustics/thermal.c','s->energy=energy;s->wall_T=wall;','s->energy=oldE;s->wall_T=wall;',['test_thermal_capacity_and_closed_books'],180),
 'missing-enthalpy-work':('core/src/gas.c','return rb_gas_u(g, T) + g->R * T;','return rb_gas_u(g, T);',['test_thermal_failure_and_hot_property_consistency'],180),
}
