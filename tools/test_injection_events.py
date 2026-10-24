"""Do not hide detached injection, missing scenarios or bad physical books."""
import copy
import math
from pathlib import Path
import unittest
from check_injection_events import commands,validate


def valid():
    return dict(ok=True,invalid_events=0,missing_click_intervals=0,phantom_click_intervals=0,
        early_heat_steps=0,dropped_events=0,diagnostic_invalid=0,positive_doses=10,clicks=10,
        step_mass_error_kg=0,step_energy_error_J=0,event_fuel_kg=.01,fuel_in_kg=.01,
        heard_fuel_kg=.01,event_sensible_J=10,fuel_sensible_J=10,mass_error_kg=0,species_error_kg=0,
        energy_error_J=0,crank_error_J=0,mass_scale_kg=.1,energy_scale_J=100,crank_scale_J=100,
        guard_counts={},scenario='loaded')


class InjectionEventTests(unittest.TestCase):
    def test_dropped_phantom_missing_and_changed_fuel_are_rejected(self):
        for key,value in [('ok',False),('dropped_events',1),('phantom_click_intervals',1),
                          ('clicks',11),('heard_fuel_kg',.011),('step_mass_error_kg',1e-12),
                          ('step_energy_error_J',1e-6),('event_sensible_J',11),('early_heat_steps',1)]:
            row=valid();row[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):validate(row)

    def test_repairs_bad_books_and_nonfinite_are_rejected(self):
        for key,value in [('mass_error_kg',1e-6),('species_error_kg',1e-6),('energy_error_J',1),
                          ('crank_error_J',1),('step_mass_error_kg',math.nan),
                          ('guard_counts',{'energy-repair':1})]:
            row=valid();row[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):validate(row)
        row=valid();row['guard_counts']={'property-extrapolation':1};validate(row)

    def test_dfco_requires_cut_resume_and_zero_clicks(self):
        row=valid();row.update(scenario='dfco-resume',fuel_cut_steps=100,cut_resumes=1,
                               fuel_cut_clicks=0,kinds={'dfco':10});validate(row)
        for key in ('fuel_cut_steps','cut_resumes'):
            bad=copy.deepcopy(row);bad[key]=0
            with self.assertRaises(ValueError):validate(bad)
        row['fuel_cut_clicks']=1
        with self.assertRaises(ValueError):validate(row)

    def test_frozen_scenarios_cover_each_engine_and_seed(self):
        jobs=commands(Path('/tmp/probe'));self.assertEqual(len(jobs),105)
        self.assertEqual(len({k for k,c in jobs}),105)
        self.assertEqual({c[c.index('--seed')+1] for k,c in jobs},{'17','29','61'})
        self.assertEqual(sum('--scenario' in c and c[c.index('--scenario')+1]=='dfco-resume' for k,c in jobs),15)


if __name__=='__main__':unittest.main()
