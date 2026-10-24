"""Reference-ingest guards: frozen sessions, engine identity, speech and audio track."""
import copy
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
from audio_wav import read_wav
import prepare_video_references as video
import reference_protocol as protocol


class VideoReferenceTests(unittest.TestCase):
    def test_new_lock_preserves_parent_and_does_not_turn_gr86_into_fb20(self):
        root=protocol.ROOT
        lock,added=video.checked_lock(root/'tools/reference_sessions_v2.json')
        self.assertEqual(len(added),3)
        self.assertEqual(sum(len(s['intervals']) for s in added),14)
        for fault in ('parent','earlier-source','engine-identity','partition'):
            bad=copy.deepcopy(lock)
            if fault=='parent':bad['parent_lock']['sha256']='0'*64
            elif fault=='earlier-source':bad['sessions'][0]['metadata']['exhaust']='invented'
            elif fault=='engine-identity':bad['sessions'][-3]['metadata']['preset_relationship']='exact-fb20'
            else:bad['sessions'][-1]['partition']='validation'
            with self.subTest(fault=fault),tempfile.TemporaryDirectory() as tmp:
                p=Path(tmp)/'lock.json';p.write_text(json.dumps(bad))
                with self.assertRaises(ValueError):video.checked_lock(p)

    def test_speech_in_either_channel_and_padding_excludes_candidate(self):
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp);session=dict(source_sha256='2'*64,source_page='https://example.invalid/?v=test',intervals=[dict(id='clip',start_frame=48000,end_frame=52800)])
            p=np.zeros((100,2));p[20,1]=.9  # at0.64s: inside the pre-window padding
            np.save(out/'test-vad.npy',p)
            (out/'speech-screen.json').write_text(json.dumps([dict(video='test',source_sha256='2'*64,model_sha256='1'*64,probabilities_sha256=protocol.digest(out/'test-vad.npy'))]))
            with self.assertRaises(ValueError):video.screen([session],out)
            p[20,1]=0;p[10,0]=.9  # at0.32s: outside the padded window
            np.save(out/'test-vad.npy',p)
            (out/'speech-screen.json').write_text(json.dumps([dict(video='test',source_sha256='2'*64,model_sha256='1'*64,probabilities_sha256=protocol.digest(out/'test-vad.npy'))]))
            self.assertEqual(video.screen([session],out)[0]['peak_speech_probability'],0)

    def test_video_decode_selects_first_audio_track_and_preserves_antiphase(self):
        ffmpeg=os.environ.get('RB_REFERENCE_FFMPEG') or shutil.which('ffmpeg')
        if not ffmpeg or subprocess.run([ffmpeg,'-version'],capture_output=True).returncode:
            self.skipTest('Set RB_REFERENCE_FFMPEG to a usable FFmpeg')
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);rate=48000;t=np.arange(rate)/rate
            tone=.2*np.sin(2*np.pi*180*t)
            original=root/'anti.wav';original.write_bytes(protocol.wav_payload(np.column_stack([tone,-tone])))
            source=root/'source.webm'
            command=[ffmpeg,'-hide_banner','-nostdin','-v','error',
                '-f','lavfi','-i','color=c=black:s=32x32:r=2:d=1',
                '-i',str(original),'-f','lavfi','-i','sine=frequency=880:duration=1:sample_rate=48000',
                '-map','0:v:0','-map','1:a:0','-map','2:a:0',
                '-c:v','libvpx-vp9','-c:a','libopus','-ac:a:1','2',
                '-disposition:a:0','0','-disposition:a:1','default','-shortest',str(source)]
            subprocess.run(command,check=True,capture_output=True)
            lock=dict(schema_version=1,sessions=[dict(id='video',family='video',partition='development',
                used_before_lock=True,source_sha256=protocol.digest(source),source={'path':'source.webm'},
                claims={'calibrated_spl':False},intervals=[dict(id='clip',start_frame=4800,end_frame=38400)])])
            with patch.object(protocol,'ROOT',root):protocol.prepare(lock,root,root/'out',Path(ffmpeg))
            x,rate=read_wav(root/'out/raw/clip.wav')
            self.assertEqual((len(x),x.shape[1],rate),(33600,2,48000))
            self.assertGreater(float(np.mean(x*x)),.01)
            self.assertLess(float(np.mean((x[:,0]+x[:,1])**2)),1e-6)


if __name__=='__main__':unittest.main()
