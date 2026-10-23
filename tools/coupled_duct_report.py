#!/usr/bin/env python3
"""Publish measured coupling fixtures in the existing single results page."""
import argparse
import base64
import json
import os
from pathlib import Path
os.environ.setdefault('MPLCONFIGDIR','/tmp/rev-bench-mpl')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from checkpoint_sections import publish_section
from run_coupled_duct import ROOT, read, exact_pressure, assess, P0


def publish(directory):
    rows=json.loads((directory/'measurements.json').read_text())
    summary=json.loads((directory/'summary.json').read_text())
    if not assess(rows)['proof_pass'] or not summary['proof_pass']:
        raise ValueError('Do not publish a failed or incomplete coupling proof')
    plt.style.use('dark_background')
    colors=['#72c9ec','#f1bc6f','#9ed69a']
    fig,axes=plt.subplots(1,2,figsize=(12,4.2),layout='constrained')
    selected=[r for r in rows if r['group']=='simple-wave' and r['amplitude']==.001]
    for color,row in zip(colors,selected):
        f=read(directory/(row['id']+'.field.csv'))
        label=f"Gas cells {row['cut']}" if row['cut']!=560 else 'All-FV control'
        axes[0].plot(f['x_m'],f['pressure_Pa']-P0,color=color,label=label)
    target=exact_pressure(f['x_m'],row['length_m'],.001,row['steps']/row['rate_Hz'])
    axes[0].plot(f['x_m'],target-P0,'--',color='white',label='Independent simple-wave solution')
    axes[0].set(xlim=(.75,1.001),xlabel='Position (m)',ylabel='Physical pressure above background (Pa)')
    axes[0].legend(fontsize=8)
    for color,cut in zip(colors,(168,280,560)):
        rr=[r for r in rows if r['group']=='simple-wave' and r['cut']==cut]
        axes[1].semilogx([r['amplitude'] for r in rr],[100*r['relative_pressure_L2'] for r in rr],
            'o-',color=color,label=f'Gas cells {cut}' if cut!=560 else 'All-FV control')
    axes[1].set(xlabel='Initial relative pressure amplitude',ylabel='Pressure waveform L2 error (%)')
    axes[1].legend(fontsize=8)
    for ax in axes:ax.grid(alpha=.15)
    fig.suptitle('Open-port homogeneous gas / characteristic duct · 192 kHz · two interface positions')
    first=directory/'coupling-accuracy.png';fig.savefig(first,dpi=150);plt.close(fig)
    fig,axes=plt.subplots(1,2,figsize=(12,4.2),layout='constrained')
    reflection=[r for r in rows if r['group']=='reflection']
    for color,row in zip(colors,reflection):
        f=read(directory/(row['id']+'.trace.csv'))
        axes[0].plot(f['time_s']*1000,f['gas_probe_Pa'],color=color,label=f"Reflection {row['reflection']:+.0f}")
    axes[0].axvline(row['expected_return_time_s']*1000,ls='--',color='white',alpha=.5)
    axes[0].set(xlim=(4.4,5.6),xlabel='Time (ms)',ylabel='Upstream gas pressure (Pa)')
    axes[0].legend(fontsize=8)
    values=summary['fixture_cost_median_seconds']
    axes[1].bar(['168 gas cells','280 gas cells','560 · all FV'],[1000*values[str(k)] for k in (168,280,560)],color=colors)
    axes[1].set(ylabel='Median wall time per 2.33 ms fixture (ms)')
    axes[1].set_title('Complete fixture cost · not a production engine speedup',fontsize=10)
    for ax in axes:ax.grid(alpha=.15)
    fig.suptitle('Returning load changes the gas · downstream nonlinear correction grid remains')
    second=directory/'coupling-feedback-cost.png';fig.savefig(second,dpi=150);plt.close(fig)
    parts=['<!-- COUPLING PROOF BEGIN -->','<section id="coupling-proof"><h2>Open-port coupling proof</h2>',
        '<p>A small retained nonlinear gas region now exchanges a single accepted transfer with '
        'a bidirectional characteristic duct. Returning pressure changes the upstream gas. '
        'Each downstream cell retains its mass, tagged species and total energy through spatial '
        'nonlinear/entropy corrections. There is no second acoustic energy reservoir.</p>',
        '<p>This is a native uniform-duct, constant-gamma, initially quiescent proof. It is separate '
        'from the static B0 graph and the production engine renderer. The accepted engine audio and '
        'all long coasts above are unchanged. Real exhaust components, mixture thermodynamics, '
        'changing thermal backgrounds, full engines and browser runtime still require validation.</p>']
    for path in (first,second):
        payload=base64.b64encode(path.read_bytes()).decode()
        parts.append(f'<img style="width:100%;height:auto" alt="{path.stem}" src="data:image/png;base64,{payload}">')
    parts.append(f'<p>Across the two gas/network interface positions, waveform error against the '
        f'independent simple-wave solution is at most {100*max(summary["partition_pressure_error"].values()):.2f}%. '
        f'Return peaks are within {summary["maximum_echo_error_ticks"]:.1f} ticks of the ideal path time. '
        'Both regions close their accepted transfer books, including real advected mass; '
        'the one-second source-free screens show no unexplained perturbation-energy growth. '
        'Fixture timings include the retained downstream correction grid and establish no engine speedup.</p>')
    parts+=['</section>','<!-- COUPLING PROOF END -->']
    publish_section(ROOT/'renders/RESULTS.html','coupling-proof','\n'.join(parts))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence',type=Path,default=ROOT/'renders/evidence/coupling-proof-2026-10-23')
    publish(parser.parse_args().evidence.resolve())
