# SPDX-License-Identifier: GPL-3.0-only
"""Packaging must retain the renderer and reject an incompatible source."""
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('trial', Path(__file__).resolve().parents[1] / 'web/prepare_display_trial.py')
trial = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trial)


class TrialTest(unittest.TestCase):
    def test_packaging(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, dest = Path(tmp) / 'published', Path(tmp) / 'trial'
            source.mkdir()
            for name in trial.ASSETS:
                (source / name).write_bytes(b'unchanged published asset')
            (source / 'remote_views.json').write_text(json.dumps({'rendererId': trial.RENDERER, 'schema': 2}))
            meta = {'source': trial.SOURCE, 'rendererId': trial.RENDERER, 'physicalFocusProtocol': 2}
            (source / 'build.json').write_text(json.dumps(meta))
            (source / 'do-not-copy.fwsc').write_bytes(b'firmware')
            trial.prepare(source, dest)
            self.assertEqual((dest / 'felucca.wasm').read_bytes(), (source / 'felucca.wasm').read_bytes())
            self.assertFalse(list(dest.glob('*.fwsc')))
            self.assertIn('audio-priority', (dest / 'index.html').read_text())
            self.assertEqual((dest / 'frame_presenter.js').read_bytes(),
                             (Path(trial.__file__).parent / 'frame_presenter.js').read_bytes())
            with self.assertRaises(FileExistsError):
                trial.prepare(source, dest)
            meta['rendererId'] = 1
            (source / 'build.json').write_text(json.dumps(meta))
            with self.assertRaises(SystemExit):
                trial.prepare(source, Path(tmp) / 'bad')
            meta['rendererId'] = trial.RENDERER
            (source / 'build.json').write_text(json.dumps(meta))
            (source / 'felucca.wasm').unlink()
            with self.assertRaises(SystemExit):
                trial.prepare(source, Path(tmp) / 'missing')


if __name__ == '__main__':
    unittest.main()
