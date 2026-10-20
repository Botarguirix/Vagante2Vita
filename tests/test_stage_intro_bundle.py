import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / 'tools' / 'stage_intro_bundle.py'


class StageIntroBundleTests(unittest.TestCase):
    def make_bundle(self, root):
        source = root / 'source'
        source.mkdir()
        files = {
            'intro-world.vwi': b'VWI2 world',
            'intro-entities.vwe': b'VWE1 entities',
            'intro-terrain.png': b'terrain',
            'intro-melee.vwa': b'melee',
            'intro-bow.vwb': b'bow',
            'knight/stand.png': b'stand',
        }
        entries = {}
        for name, data in files.items():
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            entries[name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        (source / 'manifest.json').write_text(json.dumps(entries), encoding='utf-8')
        return source, files

    def test_verifies_and_stages_all_intro_resources(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            source, files = self.make_bundle(root)
            destination = root / 'world'
            process = subprocess.run([sys.executable, str(SCRIPT), str(source),
                                      str(destination)], capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            self.assertIn(f'files={len(files)}', process.stdout)
            for name, data in files.items():
                self.assertEqual((destination / name).read_bytes(), data)

    def test_rejects_modified_intro_resource(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            source, _ = self.make_bundle(root)
            (source / 'intro-world.vwi').write_bytes(b'changed')
            process = subprocess.run([sys.executable, str(SCRIPT), str(source),
                                      str(root / 'world')], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertIn('hash mismatch', process.stderr)

    def test_rejects_unsafe_manifest_path(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            source, _ = self.make_bundle(root)
            manifest = json.loads((source / 'manifest.json').read_text(encoding='utf-8'))
            manifest['../outside'] = {'bytes': 0, 'sha256': hashlib.sha256(b'').hexdigest()}
            (source / 'manifest.json').write_text(json.dumps(manifest), encoding='utf-8')
            process = subprocess.run([sys.executable, str(SCRIPT), str(source),
                                      str(root / 'world')], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertIn('Unsafe', process.stderr)


if __name__ == '__main__':
    unittest.main()
