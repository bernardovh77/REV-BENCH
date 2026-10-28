#!/usr/bin/env python3
"""Publish qualified049 junction evidence in the single existing results page."""
import argparse,base64,io,json
from pathlib import Path
import numpy as np
from checkpoint_sections import publish_section
from refine_junction_flow import load_trace

def fragment(summary,evidence,figure=None):
    if not summary.get('pass_'):raise ValueError('Cannot publish unresolved049 numerical gates')
    candidate=summary.get('candidate','v2')
    if candidate not in ['v2','v3']:raise ValueError('Unknown qualified candidate')
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig,axes=plt.subplots(2,2,figsize=(11,7),layout='constrained')
    cases=summary['acoustic']['cases'];kinds=['straight','area','y','x','h'];positions=np.arange(5)
    for T,offset,color in [(300,-.18,'#2a78d6'),(900,.18,'#eb6834')]:
        values=[100*next(c for c in cases if c['kind']==k and c['T']==T)['candidates'][-1]['matrix_relative_error'] for k in kinds]
        axes[0,0].bar(positions+offset,values,width=.35,color=color,label=f'{T} K')
    axes[0,0].axhline(5,color='#555',ls='--',label='5% matrix gate')
    axes[0,0].set(xticks=positions,xticklabels=['Duct','Area step','Y','X','H'],ylabel='Complex-response matrix difference (%)',title='Every source, return and outlet vs independent reference');axes[0,0].legend(fontsize=8)
    for label,color,folder,name in [('4 cm stubs','#2a78d6',f'matrix-{candidate}/x-900','x-16-900-source-0-2000000'),('8 cm stubs','#eb6834',f'controls-{candidate}/edit-x','edit-x')]:
        d=load_trace(evidence/folder/(name+'.csv'));r=json.loads((evidence/folder/(name+'.json')).read_text())
        reflected=.5*(d['port_0_delta_p_Pa']+r['rho_reference']*r['c_reference']*d['port_0_outward_u_m_s'])
        axes[0,1].plot(1000*d['time_s'],reflected,color=color,label=label)
    axes[0,1].set(xlabel='Time (ms)',ylabel='Returning pressure wave (Pa)',title='Longer paths change the returning pressure wave');axes[0,1].legend(fontsize=8)
    flow=next(c for c in summary['flow'] if c['kind']=='y' and c['mode']=='reversal')
    folder=evidence/f'flow-{candidate}/y-reversal-1000'
    for n in sorted({8,16,flow['selected_n']}):
        name=f'y-reversal-1000-{n}';r=json.loads((folder/(name+'.json')).read_text());d=load_trace(folder/(name+'.csv'))
        axes[1,0].plot(1000*(d['time_s']+.5/r['rate']),1e6*(d['air_material_kg']-r['initial_mass'][0]),label=f'Subdivision {n}')
    axes[1,0].set(xlabel='Time (ms)',ylabel='Cold-material inventory change (mg)',title='Hot/cold reversal: retained coarse and finer controls');axes[1,0].legend(fontsize=8)
    d=load_trace(evidence/f'flow-{candidate}/y-flow-1000/y-flow-1000-16.csv')
    for port,label in [(0,'Driven branch'),(1,'Narrow outlet'),(2,'Other outlet')]:axes[1,1].plot(1000*d['time_s'],d[f'port_{port}_delta_p_Pa'],label=label)
    axes[1,1].set(xlabel='Time (ms)',ylabel='Pressure disturbance (Pa)',title='Finite hot-flow branches have their own response');axes[1,1].legend(fontsize=8)
    for ax in axes.flat:ax.grid(alpha=.15)
    output=io.BytesIO();fig.savefig(output,format='png',dpi=150);plt.close(fig)
    if figure:figure.write_bytes(output.getvalue())
    maximum=max(c['candidates'][-1]['matrix_relative_error'] for c in cases)
    misses=sum(not c['primary']['pass_'] for c in summary['flow'])
    return f'''<!-- JUNCTION CHECKPOINT BEGIN -->
<section id="junction-checkpoint"><h2>Stage 49 · Branches and finite exhaust junctions</h2>
<p>28 October 2026 · Reviewed numerical research checkpoint. The new reference model carries gas through finite Y, X, H and area-change geometry, including directional momentum and the pressure reflected upstream. Each connected source is checked against every return and outlet.</p>
<img style="width:100%;height:auto" alt="All-port acoustic accuracy, independent reflected-load change from longer pipes, hot/cold material convergence and distinct branch pressure responses" src="data:image/png;base64,{base64.b64encode(output.getvalue()).decode()}">
<p>Thirty source runs cover 441 complex frequency responses across the stated cold/hot bands. The largest selected response-matrix difference from the independent reference is {100*maximum:.3f}%. {misses} primary hot-flow controls require the retained finer grids to meet the unchanged material-transport gate. Clock, recording-length, conservation, reversal and long-run stability checks are recorded separately.</p>
<p><strong>Scope:</strong> controlled synthetic geometry, 200–990 K, weak subsonic flow and an adiabatic inviscid model with matched plane ports. Measured hardware losses, arbitrary installations, hotter chemistry and full-engine runtime remain later qualification work. Fine meshes are costly accuracy references. The compact approximation has measured frequency limits even with its physical pipe lengths and gas volumes retained.</p>
<p>The accepted engine examples, references and long coasts remain together on this page. Changed whole-engine comparisons belong to Stage 57 and production promotion to Stage 61. Stage 39 and the overall realism/listening gates remain open. Development is held after Stage 49.</p>
</section>
<!-- JUNCTION CHECKPOINT END -->'''

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--summary',type=Path,required=True);p.add_argument('--results',type=Path,required=True);p.add_argument('--figure',type=Path);a=p.parse_args()
    publish_section(a.results,'junction-checkpoint',fragment(json.loads(a.summary.read_text()),a.summary.parent,a.figure))
