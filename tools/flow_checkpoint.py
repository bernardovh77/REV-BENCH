#!/usr/bin/env python3
"""Publish measured 045/046 fixture evidence, preserving the accepted audio."""
import argparse
import base64
from html import escape
import io
import json
from pathlib import Path
import numpy as np
from checkpoint_sections import publish_section


def fragment(zero,flow,gain,figure=None):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    required=('gaussian_pass','carrier_pass','return_pass','contact_pass','stability_pass','zero_flow_parity_pass','conservation_pass')
    if not all(flow.get(k) for k in required) or not zero.get('physical_pass') or len(gain)!=3 or any(r['status'] for r in gain):
        raise ValueError('Do not publish a qualified checkpoint from failing/incomplete evidence')
    fig,axes=plt.subplots(2,2,figsize=(11,7.2),layout='constrained')
    ax=axes[0,0]
    for U,color in ((-70,'#d57c3c'),(0,'#4879bd'),(70,'#438b69')):
        rows=[r for r in flow['profiles'] if r['mean_flow']==U and not r['frequency']]
        ax.scatter([U]*len(rows),[100*r['waveform_l2'] for r in rows],alpha=.6,color=color)
    ax.axhline(2,color='#555',ls='--',label='2% gate');ax.legend()
    ax.set(xlabel='Frozen gas velocity (m/s)',ylabel='Pressure waveform error (%)',title='Independent finite-pulse reference')
    ax=axes[0,1]
    for U,color in ((-70,'#d57c3c'),(0,'#4879bd'),(70,'#438b69')):
        rows=[r for r in flow['returns'] if r['mean_flow']==U and r['reflection']]
        ax.scatter([r['forward_tick_error'] for r in rows],[r['return_tick_error'] for r in rows],label=f'{U:+} m/s',color=color,alpha=.7)
    ax.set(xlim=(-5,5),ylim=(-5,5),xlabel='Forward arrival error (ticks)',ylabel='Return arrival error (ticks)',title='Different forward and backward wave speeds');ax.legend()
    ax=axes[1,0];rows=flow['contacts']
    ax.bar(range(len(rows)),[100*r['contrast_transfer_relative_error'] for r in rows],color=['#d57c3c' if r['mean_flow']<0 else '#438b69' for r in rows])
    ax.axhline(5,color='#555',ls='--',label='5% contrast-transfer gate');ax.legend()
    ax.set(xlabel='Direction / temperature / interface control',ylabel='Excess-species transfer error (%)',title='Composition crosses the actual interface')
    ax=axes[1,1];rows=gain
    labels=[f'{r["mean_flow"]:+} / {r["reflection"]:+}' for r in rows]
    ax.bar(labels,[r['maximum_ratio'] for r in rows],color=['#d57c3c' if r['mean_flow']<0 else '#438b69' for r in rows])
    ax.plot(labels,[r.get('maximum_ratio_limit',1.10*max(1,r['reflection']**2*(r['c']-r['mean_flow'])/(r['c']+r['mean_flow']))) for r in rows],'k_',ms=17,label='Flow-dependent limit');ax.legend()
    ax.set(xlabel='Velocity (m/s) / pressure reflection',ylabel='Maximum / initial disturbance energy',title='Flow can transfer energy to a reflected wave')
    for ax in axes.flat:ax.grid(alpha=.15,axis='y')
    payload=io.BytesIO();fig.savefig(payload,format='png',dpi=150);plt.close(fig)
    if figure:figure.write_bytes(payload.getvalue())
    profiles=flow['profiles'];gaussians=[r for r in profiles if not r['frequency']];carriers=[r for r in profiles if r['frequency']]
    rows=[('045 · Quiescent coupling',f'{len(zero["profiles"])} independent profiles',f'{100*max(r["waveform_l2"] for r in zero["profiles"]):.3f}% worst shape error'),
          ('046 · Signed flow',f'{len(profiles)} profiles; −70 / 0 / +70 m/s',f'{100*max(r["waveform_l2"] for r in gaussians):.3f}% worst Gaussian error'),
          ('046 · Carrier transfer','200 / 2,000 / 4,000 Hz',f'{max(abs(r["magnitude_db"]) for r in carriers):.4f} dB / {max(abs(r["phase_deg"]) for r in carriers):.4f}°'),
          ('046 · Contacts',f'{len(flow["contacts"])} moving-composition cases',f'{max(r["pressure_drift_Pa"] for r in flow["contacts"]):.3g} Pa false pressure; {100*max(r["contrast_transfer_relative_error"] for r in flow["contacts"]):.3f}% transfer error')]
    table=''.join('<tr>'+''.join('<td>'+escape(str(v))+'</td>' for v in row)+'</tr>' for row in rows)
    return f'''<!-- FLOW CHECKPOINT BEGIN -->
<section id="flow-checkpoint"><h2>Stages 45–46 · Gas and returning sound waves</h2>
<p>The research prototype now passes its declared uniform-duct coupling checks with the production gas-mixture equations, fractional pipe lengths and frozen flow in both directions. These are numerical fixture results. All accepted engine clips and five long coasts remain unchanged.</p>
<img style="width:100%;height:auto" alt="Pulse error, forward and reflected travel, transported composition and one-second energy checks" src="data:image/png;base64,{base64.b64encode(payload.getvalue()).decode()}">
<table><thead><tr><th>Test</th><th>Coverage</th><th>Measured result</th></tr></thead><tbody>{table}</tbody></table>
<p>Returning pressure acts on the retained gas on the current tick. One shared transfer accounts for each species, momentum and total energy on both sides. With moving gas, acoustic mass flow includes the density change; waves travel at different speeds with and against that flow. Composition is transported rather than held behind an acoustic boundary.</p>
<p>Six one-second moving-flow cases pass their energy-growth and final-decay limits. Separate resolved-return controls show a {max(r["maximum_ratio"] for r in gain):.3f}× energy peak, close to the {gain[0]["expected_full_reflection_ratio"]:.3f}× ideal flow-dependent reflection ratio; that accounted increase is not numerical instability. A specified pressure reflection on a flowing reservoir can exchange energy with the mean flow; it is not a solid closed wall. Physical gas energy and the acoustic diagnostic are separate accounts of the same state.</p>
<p>Limits: one uniform duct; tested at 300/900 K and ±70 m/s, with weak carriers and bounded finite pulses. Weak-carrier envelope error reaches {100*max(r['waveform_l2'] for r in carriers):.3f}%; carrier magnitude/phase pass their separate frozen gates. Hot/cold fronts, evolving reversal, branches, component losses, engine integration and real-time performance remain unqualified. This full correction grid is an expensive reference, not the finished efficient renderer.</p>
<p>Stages 45–46 are complete within this scope. Stage 39 recording matching and realism/listening gates remain open. Whole-engine comparisons with the new system are planned at Stage 57; production promotion remains a later decision. Development is on hold before Stage 47.</p>
</section>
<!-- FLOW CHECKPOINT END -->'''

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--zero',type=Path,required=True);p.add_argument('--flow',type=Path,required=True)
    p.add_argument('--results',type=Path,required=True);p.add_argument('--figure',type=Path);p.add_argument('--gain',type=Path,required=True);a=p.parse_args()
    publish_section(a.results,'flow-checkpoint',fragment(json.loads(a.zero.read_text()),json.loads(a.flow.read_text()),json.loads(a.gain.read_text()),a.figure))
