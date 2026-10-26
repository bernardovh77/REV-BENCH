#!/usr/bin/env python3
"""Publish the bounded114 fixed-port proof in the existing single results page."""
import argparse,base64,io,json
from pathlib import Path
import numpy as np
from checkpoint_sections import publish_section

def fragment(summary,evidence,figure=None):
    if not summary.get('pass'):
        raise ValueError('Cannot publish a passing checkpoint from failed transport gates')
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig,axes=plt.subplots(2,2,figsize=(11,7),layout='constrained')
    for kind,color in [(0,'#2a78d6'),(1,'#eb6834')]:
        # Explicitly select one recorded clock per spatial level; do not average
        # a failed profile with a passing profile or fit the reference to it.
        selected={}
        for r in summary['grids']+summary['profiles']+summary['fine']:
            if r['kind']==kind and r['U']==70 and r['front']==1.1 and r['frequency']==4000 and not r['status']:
                if r['n'] not in selected or r['rate']<selected[r['n']]['rate']:selected[r['n']]=r
        rows=sorted(selected.values(),key=lambda r:r['n'])
        axes[0,0].plot([r['n'] for r in rows],[100*r['waveform_l2'] for r in rows],'o-',color=color,label='Cold → hot' if kind==0 else 'Hot → cold')
    axes[0,0].axhline(5,color='#555',ls='--',label='5% packet gate');axes[0,0].legend(fontsize=8)
    axes[0,0].set(xlabel='Stationary cells',ylabel='Whole-packet error (%)',title='4 kHz accuracy and retained coarse failures')
    name='fine-0-0-1.1-4000' if (evidence/'fine-0-0-1.1-4000-reference.npz').exists() else 'wave-0-0-1.1-4000'
    d=np.load(evidence/(name+'-reference.npz'))
    axes[0,1].plot(d['x'],d['reference'],color='#222',lw=2,label='Independent sharp-interface reference')
    axes[0,1].plot(d['x'],d['measured'],'--',color='#2a78d6',label='Fixed-grid candidate')
    axes[0,1].set(xlabel='Position in fixed duct (m)',ylabel='Pressure disturbance (Pa)',title='Actual reflected and transmitted packets',xlim=(.7,1.8));axes[0,1].legend(fontsize=8)
    d=np.genfromtxt(evidence/'through-70-128.csv',delimiter=',',names=True)
    axes[1,0].plot(1000*d['time_s'],1000*d['mass0_kg'],color='#2a78d6',label='Cold constituent inside')
    axes[1,0].plot(1000*d['time_s'],1000*d['mass1_kg'],color='#eb6834',label='Hot constituent inside')
    axes[1,0].set(xlabel='Time (ms)',ylabel='Material inventory (g)',title='Multiple residence times through fixed ports');axes[1,0].legend(fontsize=8)
    d=np.genfromtxt(evidence/'outlet-128-256000-0.csv',delimiter=',',names=True)
    axes[1,1].plot(1000*d['time_s'],1e6*d['mass0_kg'],color='#2a78d6')
    axes[1,1].set(xlabel='Time (ms)',ylabel='Ambient constituent inside (mg)',title='Pressure-driven inward transport through outlet')
    for ax in axes.flat:ax.grid(alpha=.15)
    output=io.BytesIO();fig.savefig(output,format='png',dpi=150);plt.close(fig)
    if figure:figure.write_bytes(output.getvalue())
    rows=summary['profiles']+summary['fine'];good=[r for r in rows if not r['status'] and r.get('packet_pass') and r.get('carrier_pass')]
    missed=sum(not r.get('packet_pass',False) or not r.get('carrier_pass',False) for r in summary['profiles'])
    books=max(max(r['energy_relative'],r['mass_relative'],r['momentum_relative']) for r in good)
    return f'''<!-- FIXED PORT CHECKPOINT BEGIN -->
<section id="fixed-port-checkpoint"><h2>Stage 114 · Gas exchange through fixed exhaust geometry</h2>
<p>26 October 2026 · Reviewed numerical research checkpoint. The new candidate keeps physical cells and ports fixed, carries distinct hot/cold gas, and shares one current mass and energy transfer across connected regions. Full and split ducts agree, including reflected load. Accepted engine clips, references and long coasts are preserved.</p>
<img style="width:100%;height:auto" alt="Fixed-grid packet convergence, independent reflected/transmitted pressure, sustained hot/cold inventory exchange and pressure-driven ambient return" src="data:image/png;base64,{base64.b64encode(output.getvalue()).decode()}">
<p>All 60 primary waveform cases are retained. {missed} primary cases require the predeclared finer correctness grid to meet the complete waveform and carrier gates. Accepted packet errors stay below {100*max(r['waveform_l2'] for r in good):.3f}%; normalized physical inventory errors in those controls stay below {books:.3g}. Thermal-contact, stationary regional flux, multi-residence throughflow, wall energy, reversal and source-free tests are recorded separately.</p>
<p><strong>Limits:</strong> two named contact constituents, 200–990 K and bounded weak subsonic flow. This pressure-equilibrium interface treatment does not model measured turbulent mixing, strong shocks, complete engines or arbitrary hot exhaust chemistry. Fine grids are accuracy references; runtime efficiency and renderer selection remain later stages. Stage 39 and engine-specific realism/listening gates remain open. Whole-engine comparisons remain Stage 57 and production promotion Stage 61.</p>
</section>
<!-- FIXED PORT CHECKPOINT END -->'''

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--summary',type=Path,required=True);p.add_argument('--results',type=Path,required=True);p.add_argument('--figure',type=Path);a=p.parse_args()
    publish_section(a.results,'fixed-port-checkpoint',fragment(json.loads(a.summary.read_text()),a.summary.parent,a.figure))
