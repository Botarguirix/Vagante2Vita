import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_asset_bundle import audit  # noqa: E402
from prepare_world_bundle import FORMAT, FORMAT_VERSION  # noqa: E402


class AssetBundleAuditTests(unittest.TestCase):
    def test_matches_original_layout_without_copying_assets(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assets = root / "assets"
            bundle = root / "bundle"
            (assets / "gfx/classes/knight/1").mkdir(parents=True)
            (assets / "gfx/monster/goblin").mkdir(parents=True)
            (assets / "gfx/items").mkdir(parents=True)
            (bundle / "knight").mkdir(parents=True)
            (bundle / "enemies/goblin").mkdir(parents=True)
            (bundle / "items").mkdir(parents=True)
            (assets / "gfx/classes/knight/1/stand.png").write_bytes(b"sprite")
            (assets / "gfx/monster/goblin/goblin_walk.png").write_bytes(b"enemy")
            (assets / "gfx/items/items-weapons.png").write_bytes(b"atlas")
            (bundle / "knight/stand.png").write_bytes(b"sprite")
            (bundle / "enemies/goblin/move.png").write_bytes(b"enemy")
            (bundle / "items/items-weapons.png").write_bytes(b"atlas")
            files = {}
            for name, content in (("knight/stand.png", b"sprite"),
                                  ("enemies/goblin/move.png", b"enemy"),
                                  ("items/items-weapons.png", b"atlas")):
                files[name] = {"bytes": len(content), "sha256": hashlib.sha256(content).hexdigest()}
            (bundle / "manifest.json").write_text(json.dumps({
                "format": FORMAT, "version": FORMAT_VERSION,
                "files": files,
            }), encoding="utf-8")
            report_path = root / "audit.json"
            report = audit(assets, bundle, report_path)
            self.assertEqual(report["source_matches"], 3)
            self.assertFalse(report["source_archive_verified"])
            saved = json.loads(report_path.read_text())
            self.assertEqual(saved["files"][0]["source_candidates"][0]["path"],
                             "gfx/monster/goblin/goblin_walk.png")
            self.assertFalse((assets / "data.vra").exists())


if __name__ == "__main__":
    unittest.main()
