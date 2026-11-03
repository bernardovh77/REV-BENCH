"""Stage 119: the research solvers' tests that can fail again.

python3 tools/plant_bugs.py 119 WORKDIR all
The gas_graph bugs name `test_gas_graph`, every gas_graph test: run against
stage 117's tests too (docs/stage-119.md), they show which ones the old
energy checks let through.
"""
BUGS = {
    # The graph's cells 1 % longer than the wave goes in a tick: the pulse
    # seems to travel 1 % fast, a quarter of its width by the time it's checked.
    'graph-sound-speed': ("experiments/acoustics/gas_graph.c",
        "s->dx=(w.c+p->mean_flow)/p->rate;",
        "s->dx=1.01*(w.c+p->mean_flow)/p->rate;",
        ['test_gas_graph'], 300),
    # The coupling hands the graph 3 % more wave than the gas sent, booking
    # what it hands over: the graph's books still balance.
    'coupling-makes-energy': ("experiments/acoustics/gas_graph.c",
        "outgoing+=(r[0].v[0]+r[1].v[0]+4*r[2].v[0])/12;",
        "outgoing+=1.03*(r[0].v[0]+r[1].v[0]+4*r[2].v[0])/12;",
        ['test_gas_graph'], 300),
    # ... or 5 % less.
    'coupling-loses-energy': ("experiments/acoustics/gas_graph.c",
        "outgoing+=(r[0].v[0]+r[1].v[0]+4*r[2].v[0])/12;",
        "outgoing+=.95*(r[0].v[0]+r[1].v[0]+4*r[2].v[0])/12;",
        ['test_gas_graph'], 300),
    # First-order upwind in the gas region's smooth interior instead of WENO5:
    # still conservative, every inventory exact, but the pulse smears.
    'first-order-gas': ("experiments/acoustics/gas_graph.c",
        "s->slope[i].v[j]=2*(j ? w[i].v[j]-face:face-w[i].v[j]);",
        "s->slope[i].v[j]=0*face;",
        ['test_gas_graph'], 300),
    # The HLLC star pressure's impedance term with its sign flipped: the
    # faces push the flow on instead of damping it, and a closed box's
    # oscillation grows.
    'closed-box-amplifies': ("experiments/acoustics/fixed_transport.c",
        "double ps=a.p+a.rho*(sl-a.u)*(sm-a.u);",
        "double ps=a.p-a.rho*(sl-a.u)*(sm-a.u);",
        ['test_junction_closed_source_free_second'], 300),
}
