#!/usr/bin/env python3
"""Publish the bounded 043/044 research checkpoint in the single results page.

Requires existing numerical evidence; never modifies measurement/listening WAVs.
"""
import argparse
import base64
from html import escape
import io
import json
from pathlib import Path
from build_contract import require
from checkpoint_sections import publish_section


def fragment(waves,mixture,geometry):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    require(len(waves)==288 and len(geometry['engines'])==5,'Incomplete checkpoint evidence')
    require(not mixture['full_screen_pass'] and all(r['failures']==0 and r['max_balance']<=1e-10 for r in geometry['engines']),
            'Checkpoint decision/evidence differs')
    fig,axes=plt.subplots(1,2,figsize=(11,3.8),layout='constrained')
    counts=[];totals=[]
    for f in (200,1000,2000,4000):
        rows=[r for r in waves if r['frequency']==f and r['representation_pass']]
        counts.append(sum(r['acoustic_pass'] for r in rows));totals.append(len(rows))
    axes[0].bar(['200 Hz','1 kHz','2 kHz','4 kHz'],[100*n/t for n,t in zip(counts,totals)],color='#4879bd')
    for i,(n,t) in enumerate(zip(counts,totals)):axes[0].text(i,100*n/t+3,f'{n}/{t}',ha='center',fontsize=10)
    axes[0].set(ylim=(0,118),ylabel='Wave tests passing (%)',title='A2: 1 dB and 10° gates over 1 m')
    contacts=[r for r in mixture['contacts'] if r['flow']==70]
    axes[1].plot([r['n'] for r in contacts],[r['pressure_drift_Pa'] for r in contacts],'o-',color='#bd5948')
    axes[1].axhline(1,color='#555',linestyle='--',label='1 Pa gate')
    axes[1].set(xlabel='Cells in contact test',ylabel='False pressure change (Pa)',title='Moving hot/cold mixture at +70 m/s')
    axes[1].annotate('Fine grid stops at 1.969 ms',xy=(320,contacts[-1]['pressure_drift_Pa']),xytext=(165,165),
                     arrowprops=dict(arrowstyle='->'),fontsize=9)
    axes[1].legend(loc='lower left');axes[1].grid(alpha=.2)
    payload=io.BytesIO();fig.savefig(payload,format='png',dpi=140);plt.close(fig)
    rows=''.join('<tr>'+''.join(f'<td>{escape(str(v))}</td>' for v in (
        r['engine'],r['components'],r['gas_cells'],f'{1000*r["volume_m3"]:.3f}',
        f'{r["connected_pairs"]}/{r["cylinder_pairs"]}'))+'</tr>' for r in geometry['engines'])
    return f'''<!-- GEOMETRY CHECKPOINT BEGIN -->
<section id="geometry-checkpoint"><h2>Stages 43–44 · Solver and exhaust geometry</h2>
<p>The new mixture solver conserves its inventories, but fails important sound-physics tests.
It remains a research prototype. The accepted engine audio and all long coasts above are unchanged.</p>
<img style="width:100%;height:auto" alt="Solver wave accuracy and moving-mixture pressure failures"
src="data:image/png;base64,{base64.b64encode(payload.getvalue()).decode()}">
<p>Stage 43: 128 of 248 representable wave cases pass; 40 additional grids are retained as
aliasing controls. Moving mixtures create 117–155 Pa false pressure changes. This candidate
is rejected for production. Conservation alone does not establish realistic sound.</p>
<p>Stage 44: shared exhaust descriptions now compile into complete branch/port plans,
gas storage and an executable lossless geometry test. All 42 connected cylinder/outlet pairs
respond; all 14 independent-bank pairs stay silent. Dimensions remain modeled estimates
where measurements are unavailable.</p>
<table><thead><tr><th>Engine</th><th>Pipe regions</th><th>Gas cells</th><th>Modeled volume (L)</th><th>Connected pairs</th></tr></thead><tbody>{rows}</tbody></table>
<p>Catalyst, muffler, packing, junction and outlet identities are retained. Their full losses,
hot-flow behavior, engine feedback and real installation routes need later qualification.
Missing 3D centerlines fail spatial preparation; synthetic bent routes pass length and connection
checks. This checkpoint adds no invented exhaust tone or measured-hardware claim.</p>
<p>14 deliberately planted solver/geometry faults were caught. Stage 39 reference matching,
listening acceptance and real-time engine delivery remain open. Stage 45 has not started.</p>
</section>
<!-- GEOMETRY CHECKPOINT END -->'''


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--waves',type=Path,required=True);parser.add_argument('--mixture',type=Path,required=True)
    parser.add_argument('--geometry',type=Path,required=True);parser.add_argument('--results',type=Path,required=True)
    a=parser.parse_args();publish_section(a.results,'geometry-checkpoint',fragment(*(
        json.loads(p.read_text()) for p in (a.waves,a.mixture,a.geometry))))
