"""Independent reference and reporting gates must detect bad coupling evidence."""
import copy
import unittest
import numpy as np
from run_coupled_duct import assess, exact_pressure, C, P0, GAMMA, RHO


def fixture():
    rows=[]
    base=dict(status=0,failures=0,failed_stage=0,left_balance=[0]*4,right_balance=[0]*4,
              cells=560,rate_Hz=192000.,length_m=560*343/192000.)
    for amplitude in (1e-5,.001,.01):
        for cut in (168,280,560):
            rows.append(dict(base,id=f'w-{amplitude}-{cut}',group='simple-wave',cut=cut,
                amplitude=amplitude,relative_pressure_L2=.01,steps=448,reflection=0.))
    for r in (-1.,0.,1.):
        rows.append(dict(base,id=f'r-{r}',group='reflection',reflection=r,steps=1120,cut=168,amplitude=.001,
            echo_error_ticks=2.5,return_peak_Pa=100*r))
        rows.append(dict(base,id=f'g-{r}',group='growth',reflection=r,steps=192000,cut=168,amplitude=.001,
            initial_perturbation_J=1e-6,peak_perturbation_J=1e-6,final_perturbation_J=1e-12))
    return [copy.deepcopy(row) for row in rows]


class CoupledProofTests(unittest.TestCase):
    def test_independent_characteristic_peak_before_shock(self):
        amplitude=.001; time=.0005; length=1.
        sound=C*(1+amplitude)**((GAMMA-1)/(2*GAMMA))
        velocity=2*(sound-C)/(GAMMA-1)
        x=.15*length+(C+(GAMMA+1)*velocity/2)*time
        expected=P0*(1+amplitude)
        self.assertAlmostEqual(float(exact_pressure(np.array([x]),length,amplitude,time)[0]),expected,places=8)

    def test_growth_and_flux_corruption_cannot_be_promoted(self):
        rows=fixture()
        self.assertTrue(assess(rows)['gates']['source_free_growth'])
        rows[-1]['peak_perturbation_J']=1.1e-6
        self.assertFalse(assess(rows)['proof_pass'])
        rows=fixture();rows[0]['right_balance'][2]=1e-5
        self.assertFalse(assess(rows)['proof_pass'])

    def test_missing_or_duplicate_cases_are_rejected(self):
        rows=fixture()
        with self.assertRaises(ValueError):assess(rows[:-1])
        with self.assertRaises(ValueError):assess(rows+[rows[0]])
        rows=fixture();rows[1]['relative_pressure_L2']=float('nan')
        with self.assertRaises(ValueError):assess(rows)
        rows=fixture();rows[-1]['steps']=128
        with self.assertRaises(ValueError):assess(rows)


if __name__=='__main__': unittest.main()
