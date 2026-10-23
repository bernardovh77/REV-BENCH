"""Section publication must retain a rebuildable checkpoint after I/O failure."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from checkpoint_sections import compose, load, publish_section


def section(label):
    return ('<!-- AUDIO EXPERIMENT BEGIN -->'
            f'<section id="audio-experiment">{label}</section>'
            '<!-- AUDIO EXPERIMENT END -->')


class CheckpointSectionTests(unittest.TestCase):
    def test_fragment_fingerprint_is_portable_across_text_defaults(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            page = root / 'RESULTS.html'
            page.write_text('<html><nav></nav><main></main></html>')
            original = Path.write_text

            def windows_text_defaults(path, text, *args, **kwargs):
                if kwargs.get('newline') is None:
                    text = text.replace('\n', '\r\n')
                kwargs.setdefault('encoding', 'cp1252')
                return original(path, text, *args, **kwargs)

            with patch.object(Path, 'write_text', windows_text_defaults):
                publish_section(page, 'audio-experiment', section('échappement\n'))
            self.assertIn('échappement', page.read_bytes().decode('utf-8'))
            self.assertEqual(compose(page.read_text(encoding='utf-8'), root), page.read_text(encoding='utf-8'))

    def test_failed_publication_preserves_manifest_page_and_other_sections(self):
        for failure in ('page-write', 'manifest-write', 'manifest-install', 'page-install'):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                page = root / 'RESULTS.html'
                page.write_text('<html><nav></nav><main></main></html>')
                publish_section(page, 'audio-experiment', section('accepted'))
                other = '<!-- COUPLING PROOF BEGIN --><section id="coupling-proof">proof</section><!-- COUPLING PROOF END -->'
                publish_section(page, 'coupling-proof', other)
                before = page.read_bytes()
                manifest_before = load(root)
                write, replace = Path.write_text, Path.replace

                def failing_write(path, text, *args, **kwargs):
                    suffix = '.checkpoint.pending.html' if failure == 'page-write' else '.pending.json'
                    if failure.endswith('write') and path.name.endswith(suffix):
                        write(path, 'partial')
                        raise OSError('publication failure')
                    return write(path, text, *args, **kwargs)

                def failing_replace(path, target):
                    name = 'RESULTS.html' if failure == 'page-install' else 'manifest.json'
                    if failure.endswith('install') and Path(target).name == name:
                        raise OSError('publication failure')
                    return replace(path, target)

                with patch.object(Path, 'write_text', failing_write), patch.object(Path, 'replace', failing_replace):
                    with self.assertRaisesRegex(OSError, 'publication failure'):
                        publish_section(page, 'audio-experiment', section('replacement'))
                self.assertEqual(page.read_bytes(), before)
                self.assertEqual(load(root), manifest_before)
                self.assertEqual(compose(page.read_text(), root), before.decode())
                self.assertFalse(list(root.rglob('*.pending.*')))
                publish_section(page, 'audio-experiment', section('replacement'))
                self.assertIn(other, page.read_text())
                self.assertIn('replacement', page.read_text())

    def test_interrupted_section_write_preserves_rebuild_and_retry(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            page = root/'RESULTS.html'
            page.write_text('<html><nav></nav><main></main></html>')
            publish_section(page, 'audio-experiment', section('accepted'))
            previous = page.read_bytes()
            original = Path.write_text

            def interrupted_write(path, text, *args, **kwargs):
                if root/'checkpoint/sections' in path.parents and path.suffix == '.html':
                    original(path, 'interrupted fragment')
                    raise OSError('simulated interrupted section write')
                return original(path, text, *args, **kwargs)

            with patch.object(Path, 'write_text', interrupted_write):
                with self.assertRaisesRegex(OSError, 'interrupted section'):
                    publish_section(page, 'audio-experiment', section('replacement'))

            self.assertEqual(page.read_bytes(), previous)
            self.assertEqual(compose(page.read_text(), root), previous.decode())
            publish_section(page, 'audio-experiment', section('replacement'))
            self.assertIn('replacement', page.read_text())
            self.assertNotIn('accepted', page.read_text())
            self.assertEqual(page.read_text().count('id="audio-experiment"'), 1)


if __name__ == '__main__':
    unittest.main()
