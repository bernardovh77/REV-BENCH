#!/usr/bin/env python3
"""Consolidate the measured047/048 physics checkpoint without changing audio."""
import argparse,base64,io,json
from pathlib import Path
import numpy as np
from checkpoint_sections import publish_section


def fragment(properties,transport,evidence,refinement,refinement_evidence,figure=None):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    required=('primary_pass','contact_pass','reversal_pass','wall_pass','stability_pass','conservation_pass','guard_pass')
    if not all(transport.get(k) for k in required) or not properties['internal_pass'] or not properties['wall_pass']:
        raise ValueError('Incomplete or failing qualification evidence')
    fig,axes=plt.subplots(2,2,figsize=(11,7),layout='constrained')
    rows=[r for r in properties['nasa'] if r['mixture']==2]
    axes[0,0].plot([r['T'] for r in rows],[100*r['cp_error'] for r in rows],'o-',color='#bf644b')
    axes[0,0].axvspan(1000,4000,color='#bf644b',alpha=.08)
    axes[0,0].set(xlabel='Gas temperature (K)',ylabel='Heat-capacity difference (%)',title='Apparent combustion fit vs frozen products')
    axes[0,0].annotate('Different closures; hot acoustic validity open',xy=(2500,36),xytext=(1150,120),fontsize=9,arrowprops=dict(arrowstyle='->'))
    wall=properties['sensitivities']
    axes[0,1].bar(['Gas minimum','Gas maximum','Wall minimum','Wall maximum'],[min(r['gas_time_s'] for r in wall),max(r['gas_time_s'] for r in wall),min(r['wall_time_s'] for r in wall),max(r['wall_time_s'] for r in wall)],color=['#4879bd']*2+['#c47c42']*2)
    axes[0,1].set(yscale='log',ylabel='Scenario time constant (seconds)',title='Gas and metal respond on different timescales')
    axes[0,1].tick_params(axis='x',labelrotation=12)
    a=np.load(refinement_evidence/'packet-0-3072-reference.npz');coarse=np.load(refinement_evidence/'packet-0-768-reference.npz')
    axes[1,0].plot(a['x'],a['reference'],color='#222',lw=2,label='Independent reference')
    axes[1,0].plot(coarse['x'],coarse['measured'],color='#c47c42',lw=1,label='768 cells')
    axes[1,0].plot(a['x'],a['measured'],'--',color='#4879bd',label='3,072 cells')
    axes[1,0].set(xlabel='Physical position (m)',ylabel='Pressure disturbance (Pa)',title='4 kHz packet convergence beyond a carrier score',xlim=(.9,1.9));axes[1,0].legend(fontsize=9)
    d=np.genfromtxt(evidence/'outlet.csv',delimiter=',',names=True)
    ax=axes[1,1];ax.plot(1000*d['time_s'],d['port_u_m_s'],color='#4879bd',label='Outlet gas velocity')
    ax.axhline(0,color='#777',lw=.7);ax.set(xlabel='Time (ms)',ylabel='Gas velocity (m/s)',title='Pressure-driven reversal draws in ambient air')
    twin=ax.twinx();ambient=d['air_left_kg']-(.1/.899)*d['burned_left_kg']
    twin.plot(1000*d['time_s'],1e6*ambient,color='#c47c42',label='Ambient air inside');twin.set_ylabel('Ambient air inside (mg)',color='#a66632')
    ax.legend(loc='upper left',fontsize=8);twin.legend(loc='upper right',fontsize=8)
    for ax in axes.flat:ax.grid(alpha=.15)
    output=io.BytesIO();fig.savefig(output,format='png',dpi=150);plt.close(fig)
    if figure:figure.write_bytes(output.getvalue())
    if not refinement['convergent']:raise ValueError('Packet refinement did not converge')
    fine=[r for r in refinement['rows'] if r['n']==3072]
    primary=transport['profiles'][:60];gaussian=max(r['waveform_l2'] for r in primary if not r['frequency'])
    carrier=[q for r in primary if r.get('packets') for q in r['packets']]
    return f'''<!-- THERMAL CHECKPOINT BEGIN -->
<section id="thermal-checkpoint"><h2>Stages 47–48 · Thermal memory, fronts and reversal</h2>
<p>New numerical checkpoint · 25 October 2026. The prototype now carries distinct hot/cold gas and composition while pressure waves propagate through them. A finite wall stores heat separately and stays fixed in space. Accepted engine clips, references and all five long coasts remain unchanged.</p>
<img style="width:100%;height:auto" alt="Gas-property limits, gas and wall timescales, reflected/transmitted pulse and actual ambient entrainment on reversal" src="data:image/png;base64,{base64.b64encode(output.getvalue()).decode()}">
<p><strong>Stage 47:</strong> the property/energy audit and independent low-temperature reference checks pass. Scenario gas response times are 0.075–0.30 seconds; wall response times are 16–133 seconds. These material and heat-transfer scenarios are not measured engine presets. The high-temperature combustion heat-capacity fit is not calibrated exhaust acoustics; that remains a qualification requirement.</p>
<p><strong>Stage 48:</strong> all 60 primary hot/cold scattering profiles pass their declared tests at 768 cells and 768 kHz. Worst Gaussian shape error is {100*gaussian:.3f}%; carrier error stays within {max(abs(r['magnitude_db']) for r in carrier):.4f} dB and {max(abs(r['phase_deg']) for r in carrier):.3f}°. Composition/enthalpy transport, pressure-driven ambient entrainment, fixed-wall energy and two one-second stability screens pass. Physical energy/work accounts close separately from acoustic diagnostics.</p>
<p>A useful failed control is retained: positive pressure first pushes hot gas outside, so subsequent negative flow can return that same hot gas without drawing in ambient air. The suction-first test above demonstrates actual cold-air transport. Negative velocity alone is insufficient evidence of ambient entrainment.</p>
<p><strong>Limits:</strong> this new material-coordinate reference supports 200–990 K, bounded weak waves and a uniform-area duct with explicit one-dimensional reservoir/buffer boundaries. It is not an evolving B0 delay adapter or a production engine renderer. Mesh/remapping, high-temperature gas closure, branches, components, physical outlet radiation and whole-engine cost still need qualification. At 768 cells, complete 4 kHz packet error reaches 20.65% even while the carrier test passes. Supplemental 3,072-cell controls reduce it to {100*max(r["waveform_l2"] for r in fine):.3f}% at much higher cost; these fine grids are accuracy references, not an efficient production solution. The accepted sound is not inferred from these plots.</p>
<p>The plan, source/tests, notes and Graphify are updated. Stages 47–48 are reviewed within their documented research scope. A new Stage 114 must qualify the bridge to fixed exhaust geometry before Stage 49; the moving-material reference does not supply that production interface. Development is on hold after Stage 48. Stage 39 reference matching and realism/listening/runtime gates remain open. Whole-engine new-system sound comparisons remain planned for Stage 57.</p>
</section>
<!-- THERMAL CHECKPOINT END -->'''

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--properties',type=Path,required=True);p.add_argument('--transport',type=Path,required=True)
    p.add_argument('--refinement',type=Path,required=True);p.add_argument('--results',type=Path,required=True);p.add_argument('--figure',type=Path);a=p.parse_args()
    publish_section(a.results,'thermal-checkpoint',fragment(json.loads(a.properties.read_text()),json.loads(a.transport.read_text()),a.transport.parent,json.loads(a.refinement.read_text()),a.refinement.parent,a.figure))
