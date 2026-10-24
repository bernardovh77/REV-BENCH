"""Stage041 report gates must retain failed numerical evidence."""
import copy
import math
import unittest
from check_combustion_variation import commands,validate
from pathlib import Path


def valid_row():
    stat=lambda mean:dict(n=10,mean=mean,sd=1,min=mean-1,max=mean+1,cov_pct=1)
    return dict(ok=True,warm_stalled_steps=0,early_heat_steps=0,invalid_events=0,dropped_events=0,
        diagnostic_invalid=0,flags=[0]*11,guard_counts={},mass_error_kg=0,species_error_kg=0,
        energy_error_J=0,crank_error_J=0,mass_scale_kg=.01,energy_scale_J=1e5,crank_scale_J=1e4,
        cylinders=[dict(net_imep_Pa=stat(100000),gross_imep_Pa=stat(150000),pumping_mep_Pa=stat(50000))])


class CombustionVariationTests(unittest.TestCase):
    def test_failed_native_events_books_and_repairs_are_rejected(self):
        changes=[('ok',False),('early_heat_steps',1),('warm_stalled_steps',1),
                 ('dropped_events',1),('mass_error_kg',1e-9),('energy_error_J',.01),
                 ('crank_error_J',.1),('diagnostic_invalid',1)]
        for key,value in changes:
            row=valid_row();row[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):validate(row)
        row=valid_row();row['guard_counts']['species-repair']=1
        with self.assertRaises(ValueError):validate(row)
        row=valid_row();row['guard_counts']['invalid-flow-reset']=1
        with self.assertRaises(ValueError):validate(row)

    def test_work_window_and_nonfinite_metric_are_rejected(self):
        row=valid_row();row['cylinders'][0]['gross_imep_Pa']['mean']+=100
        with self.assertRaises(ValueError):validate(row)
        row=valid_row();row['cylinders'][0]['pumping_mep_Pa']['n']=9
        with self.assertRaises(ValueError):validate(row)
        row=valid_row();row['cylinders'][0]['net_imep_Pa']['sd']=math.nan
        with self.assertRaises(ValueError):validate(row)

    def test_extrapolation_is_reported_without_invented_extinction(self):
        row=valid_row();row['flags'][3]=10
        self.assertEqual(validate(row)['flags'][3],10)
        row['flags'][7]=1
        with self.assertRaises(ValueError):validate(row)

    def test_predeclared_map_covers_all_presets_and_seeds(self):
        jobs=commands(Path('/tmp/native-probe'))
        self.assertEqual(len(jobs),120)
        self.assertEqual(len({key for key,cmd in jobs}),120)
        idles=[cmd for key,cmd in jobs if '-idle-' in key]
        self.assertEqual(len(idles),15)
        for cmd in idles:
            self.assertEqual(cmd[cmd.index('--seconds')+1],'30')
            self.assertEqual(cmd[cmd.index('--warmup')+1],'6')
        self.assertEqual({cmd[cmd.index('--seed')+1] for key,cmd in jobs},{'17','29','61'})


if __name__=='__main__':unittest.main()
