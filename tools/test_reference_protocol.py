import copy
import unittest
import numpy as np
from reference_protocol import validate,measurements,audition_gains,wav_payload


def fixture():
    return dict(schema_version=1,sessions=[dict(id='recording-a',family='session-a',partition='development',used_before_lock=True,source_sha256='1'*64,metadata={},claims={'calibrated_spl':False},intervals=[dict(id='clip-a',start_frame=0,end_frame=48000)])])


class ReferenceProtocolTests(unittest.TestCase):
    def test_original_session_and_fingerprint_cannot_leak(self):
        a=fixture();validate(a)
        for fault in ('seen-validation','same-family','same-content'):
            x=copy.deepcopy(a)
            if fault=='seen-validation':x['sessions'][0]['partition']='validation'
            else:
                b=copy.deepcopy(x['sessions'][0]);b.update(id='recording-b',partition='validation',used_before_lock=False);b['intervals'][0]['id']='clip-b'
                if fault=='same-family':b['source_sha256']='2'*64
                else:b['family']='session-b'
                x['sessions'].append(b)
            with self.subTest(fault=fault),self.assertRaises(ValueError):validate(x)

    def test_unknown_geometry_cannot_support_calibrated_spl(self):
        x=fixture();x['sessions'][0]['claims']['calibrated_spl']=True
        with self.assertRaises(ValueError):validate(x)

    def test_unsafe_ids_and_invalid_sample_intervals_reject(self):
        for key,value in [('id','../accepted'),('start_frame',.5),('end_frame',-1)]:
            x=fixture();x['sessions'][0]['intervals'][0][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):validate(x)

    def test_antiphase_channels_keep_their_measured_power(self):
        t=np.arange(48000)/48000;x=.2*np.sin(2*np.pi*100*t);r=measurements(np.column_stack([x,-x]))
        np.testing.assert_allclose(r['channel_power'],[.02,.02],rtol=1e-12)
        self.assertAlmostEqual(r['mean_channel_power'],.02)
        self.assertEqual(r['folded_mono_power'],0)
        self.assertAlmostEqual(r['mean_channel_band_power'][0],.02,places=6)

    def test_common_gain_preserves_level_matching_explicitly_removes_ratio(self):
        t=np.arange(48000)/48000;x=np.sin(2*np.pi*440*t)[:,None];clips=[x*.2,x*.05]
        a=audition_gains(clips,'common');b=audition_gains(clips,'level-matched')
        self.assertEqual(a[0],a[1])
        pa=[np.mean((x*g)**2) for x,g in zip(clips,a)];pb=[np.mean((x*g)**2) for x,g in zip(clips,b)]
        self.assertAlmostEqual(pa[0]/pa[1],16);self.assertAlmostEqual(pb[0]/pb[1],1)
        for x,g in zip(clips,b):self.assertLessEqual(float(np.max(abs(x*g))),.8)

    def test_listening_conversion_rejects_clipping_measurement_keeps_it(self):
        x=np.array([[2.,-2.],[0.,0.]])
        self.assertIn(b'WAVE',wav_payload(x))
        np.testing.assert_array_equal(np.frombuffer(wav_payload(x)[44:],dtype='<f4'),[2,-2,0,0])
        with self.assertRaises(ValueError):wav_payload(x,1)


if __name__=='__main__':unittest.main()
