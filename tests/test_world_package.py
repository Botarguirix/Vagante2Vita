import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from tools.prepare_world_bundle import FORMAT, FORMAT_VERSION, validate_manifest
from tools.prepare_act_one_campaign import prepare as prepare_campaign
from tools.prepare_act_one_campaign import load_route_proof
from tools.record_act_one_route import record


class WorldPackageTests(unittest.TestCase):
    def make_package(self, root, name="intro-world.vwi", data=b"world"):
        target=root/name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(data)
        (root/"manifest.json").write_text(json.dumps({
            "format":FORMAT,"version":FORMAT_VERSION,"files":{
                name:{"bytes":len(data),"sha256":hashlib.sha256(data).hexdigest()}
            }
        }),encoding="utf-8")

    def test_validates_external_package(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);self.make_package(root)
            self.assertEqual(len(validate_manifest(root)["files"]),1)

    def test_rejects_changed_asset(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);self.make_package(root);(root/"intro-world.vwi").write_bytes(b"changed")
            with self.assertRaisesRegex(ValueError,"Size/hash mismatch"):
                validate_manifest(root)

    def test_rejects_escaping_path(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            (root/"manifest.json").write_text(json.dumps({
                "format":FORMAT,"version":FORMAT_VERSION,"files":{
                    "../outside":{"bytes":0,"sha256":hashlib.sha256(b"").hexdigest()}
                }
            }),encoding="utf-8")
            with self.assertRaisesRegex(ValueError,"Unsafe"):
                validate_manifest(root)

    def test_rejects_unknown_version(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);self.make_package(root)
            doc=json.loads((root/"manifest.json").read_text());doc["version"]=999
            (root/"manifest.json").write_text(json.dumps(doc),encoding="utf-8")
            with self.assertRaisesRegex(ValueError,"Unsupported"):
                validate_manifest(root)

    def test_prepares_act_one_packets_with_vwe2_vwe3_and_runtime_filenames(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);assets=root/"assets"
            for cls in ("knight","rogue","mage"):
                for anim in ("stand","walk","jump","fall","duck","climb","lookup","hang"):
                    path=assets/"gfx"/"classes"/cls/"1"/(anim+".png")
                    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b"sprite")
            for rel in (
                "gfx/chest_small.png","gfx/traps/spikes.png","gfx/traps/spikes_broken.png",
                "gfx/traps/arrow_trap.png","gfx/traps/trap_trigger.png","gfx/traps/boulder_vine.png",
                "gfx/traps/boulder_blocker.png","gfx/traps/boulders.png",
                "gfx/traps/shake_boulder.png",
                "gfx/doodads/breakable_wall.png",
                "gfx/scenes/intro/entities/little_firefly.png",
            ):
                path=assets/rel;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b"art")
            for name in ("counter.png","shop_sign.png","torch.png","decorative_helmet.png",
                         "decorative_shield.png","flower_1.png","spellbook_altar.png",
                         "waterfall_1_2.png","waterfall_1_3.png","waterfall_1_4.png"):
                path=assets/"gfx"/"doodads"/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b"art")
            enemy_files={
                "goblin":("goblin_stand.png","goblin_walk.png","goblin_jump.png","goblin_hurt.png","goblin_attack.png"),
                "bat":("bat_hang.png","bat.png","bat.png","bat_hurt.png","bat.png"),
                "lurker":("lurker_retract.png","lurker_attack.png","lurker_retract.png","lurker_retract.png","lurker_attack.png"),
                "slime":("stand.png","walk.png","jump.png","jump.png","walk.png"),
                "spider":("spider_idle.png","spider_walk.png","spider_jump.png","spider_hurt.png","spider_webshot.png"),
            }
            for family,filenames in enemy_files.items():
                for filename in filenames:
                    path=assets/"gfx"/"monster"/family/filename
                    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b"enemy art")
            packet=root/"world.vwi";packet.write_bytes(b"VWI2")
            terrain=root/"terrain.png";terrain.write_bytes(b"PNG")
            entities=root/"entities.vwe";entities.write_bytes(struct.pack("<4sHH",b"VWE2",1,16)+bytes(16))
            entities.with_name("act-one-enemies.vwe").write_bytes(struct.pack("<4sHH",b"VWE3",0,16))
            report=root/"act-one-export.json";report.write_text(json.dumps({
                "floor_complete":False,"exact_generation_parity":False,
                "entities_instantiated":False,"native_player_route_verified":False,
                "native_exit_transition_verified":False}),encoding="utf-8")
            loot=root/"loot.vlt"
            loot.write_bytes(
                struct.pack("<4sHHHHH",b"VLT1",2,1,1,1,1)
                +struct.pack("<BBHH",15,0,0,1)
                +struct.pack("<IIHHBBH",0,10,1,49,0,1,12)
                +struct.pack("<BBHH",23,0,0,1)
                +struct.pack("<BBI",15,0,1000))
            for name in ("items.png","items-weapons.png","items-armor.png","potions.png",
                         "scrolls.png","spell_books.png","items-explosives.png"):
                path=assets/"gfx"/"items"/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b"atlas")
            source_archive=root/"data.vra";source_archive.write_bytes(b"private source archive")
            output=root/"world"/"act-one"
            subprocess.run([sys.executable,str(Path(__file__).resolve().parents[1]/"tools"/"prepare_world_bundle.py"),
                str(assets),str(packet),str(terrain),str(output),"--act-one","--entity-packet",str(entities),
                "--act-one-report",str(report),"--loot-tables",str(loot),"--source-archive",str(source_archive)],
                check=True,capture_output=True,text=True)
            manifest=validate_manifest(output)
            self.assertEqual(manifest["source"],{
                "kind":"vagante-data-vra","bytes":len(b"private source archive"),
                "sha256":hashlib.sha256(b"private source archive").hexdigest(),"included":False})
            self.assertFalse((output/"data.vra").exists())
            self.assertIn("act-one-world.vwi",manifest["files"])
            self.assertIn("act-one-entities.vwe",manifest["files"])
            self.assertIn("act-one-enemies.vwe",manifest["files"])
            self.assertIn("entities/spikes.png",manifest["files"])
            self.assertIn("entities/boulders.png",manifest["files"])
            self.assertIn("entities/shake_boulder.png",manifest["files"])
            self.assertIn("entities/breakable_wall.png",manifest["files"])
            self.assertIn("entities/torch.png",manifest["files"])
            self.assertIn("entities/flower_1.png",manifest["files"])
            self.assertIn("entities/little_firefly.png",manifest["files"])
            self.assertIn("enemies/goblin/attack.png",manifest["files"])
            self.assertIn("item-loot.vlt",manifest["files"])
            self.assertIn("items/items.png",manifest["files"])
            self.assertIn("items/items-explosives.png",manifest["files"])
            self.assertIn("entities/doodads/waterfall_1_4.png",manifest["files"])
            for path in ("enemies/goblin/stand.png","enemies/goblin/move.png",
                         "enemies/bat/air.png","enemies/lurker/hurt.png",
                         "enemies/slime/air.png","enemies/spider/stand.png",
                         "enemies/spider/attack.png"):
                self.assertIn(path,manifest["files"])
            self.assertIn("act-one-export.json",manifest["files"])

    def test_rejects_act_one_report_claiming_completion_without_evidence(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);assets=root/"assets";assets.mkdir()
            packet=root/"world.vwi";packet.write_bytes(b"VWI2")
            terrain=root/"terrain.png";terrain.write_bytes(b"PNG")
            report=root/"act-one-export.json";report.write_text(json.dumps({
                "floor_complete":True,"exact_generation_parity":False,
                "entities_instantiated":False,"native_player_route_verified":False,
                "native_exit_transition_verified":False}),encoding="utf-8")
            output=root/"world"/"act-one"
            result=subprocess.run([sys.executable,str(Path(__file__).resolve().parents[1]/"tools"/"prepare_world_bundle.py"),
                str(assets),str(packet),str(terrain),str(output),"--act-one","--act-one-report",str(report)],
                capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertIn("claims completion without all required evidence",result.stderr)

    def make_floor_export(self, root, name):
        export = root / name
        export.mkdir()
        (export / "act-one-world.vwi").write_bytes(
            struct.pack("<4sHHII4HII", b"VWI2", 1, 1, 0, 1, 0, 0, 0, 0, 0, 0)
            + struct.pack("<bB5h", 0, 0, 0, 0, 0, 0, 0))
        (export / "act-one-entities.vwe").write_bytes(struct.pack("<4sHH", b"VWE2", 0, 16))
        (export / "act-one-enemies.vwe").write_bytes(struct.pack("<4sHH", b"VWE3", 0, 16))
        (export / "intro-terrain.png").write_bytes(b"png-fixture")
        (export / "act-one-layout.json").write_text("{}", encoding="utf-8")
        (export / "act-one-export.json").write_text(json.dumps({
            "floor_complete": False, "exact_generation_parity": False,
            "native_player_spawn_geometry": {"verified": True},
            "native_tile_path": {"reachable": True},
        }), encoding="utf-8")
        return export

    def test_prepares_two_more_private_act_one_floors_and_hashes_every_file(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            base = root / "base-act-one"
            base.mkdir()
            self.make_package(base, "act-one-world.vwi", b"VWI2-base")
            loot=struct.pack("<4sHHHHH",b"VLT1",2,1,1,1,1)
            loot+=struct.pack("<BBHH",15,0,0,1)
            loot+=struct.pack("<IIHHBBH",0,10,1,49,0,1,12)
            loot+=struct.pack("<BBHH",23,0,0,1)
            loot+=struct.pack("<BBI",15,0,1000)
            (base/"item-loot.vlt").write_bytes(loot)
            manifest=json.loads((base/"manifest.json").read_text("utf-8"))
            manifest["files"]["item-loot.vlt"]={"bytes":len(loot),"sha256":hashlib.sha256(loot).hexdigest()}
            (base/"manifest.json").write_text(json.dumps(manifest),encoding="utf-8")
            floor1 = self.make_floor_export(root, "export-1")
            floor2 = self.make_floor_export(root, "export-2")
            output = root / "campaign"
            prepare_campaign(base, output, [(1, floor1), (2, floor2)])
            manifest = validate_manifest(output)
            self.assertIn("floors/floor-01.vwi", manifest["files"])
            self.assertIn("floors/floor-02-entities.vwe", manifest["files"])
            self.assertIn("item-loot.vlt", manifest["files"])
            self.assertEqual((output/"item-loot.vlt").read_bytes(),loot)
            campaign = json.loads((output / "campaign.json").read_text("utf-8"))
            self.assertEqual(campaign["loot_table"]["sha256"],hashlib.sha256(loot).hexdigest())
            self.assertEqual(campaign["loot_table"]["rarity_rows"],1)
            self.assertEqual(set(campaign["floors"]), {"0", "1", "2"})
            self.assertFalse(campaign["campaign_complete"])
            self.assertFalse(campaign["floors"]["1"]["floor_complete"])
            self.assertIn("runtime next-floor transition is implemented",
                          campaign["caveat"])
            self.assertIn("successful Vita3K replay", campaign["caveat"])

    def test_campaign_packages_and_validates_controller_trace_sidecar(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            base = root / "base-act-one"
            base.mkdir()
            self.make_package(base, "act-one-world.vwi", b"VWI2-base")
            loot = (struct.pack("<4sHHHHH", b"VLT1", 2, 1, 1, 1, 1)
                    + struct.pack("<BBHH", 15, 0, 0, 1)
                    + struct.pack("<IIHHBBH", 0, 10, 1, 49, 0, 1, 12)
                    + struct.pack("<BBHH", 23, 0, 0, 1)
                    + struct.pack("<BBI", 15, 0, 1000))
            (base / "item-loot.vlt").write_bytes(loot)
            manifest = json.loads((base / "manifest.json").read_text("utf-8"))
            manifest["files"]["item-loot.vlt"] = {
                "bytes": len(loot), "sha256": hashlib.sha256(loot).hexdigest()}
            (base / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            floor1 = self.make_floor_export(root, "export-1")
            floor2 = self.make_floor_export(root, "export-2")
            trace = root / "route.vtr"
            trace.write_bytes(struct.pack("<4sI4H", b"VTR1", 4, 0x80, 0x80, 0x20, 0x20))
            log = root / "route.log"
            log.write_text("ACT_ONE_ROUTE seed=12345 tiles=3 waypoint=2 ticks=4 jumps=0 reached_exit=1",
                           encoding="utf-8")
            proof_path = root / "floor-01.json"
            record(log, floor1 / "act-one-world.vwi", floor1 / "act-one-entities.vwe",
                   proof_path, 12345, trace_path=trace)
            output = root / "campaign"
            prepare_campaign(base, output, [(1, floor1), (2, floor2)], {1: proof_path})
            manifest = validate_manifest(output)
            self.assertIn("route-proofs/floor-01.json", manifest["files"])
            self.assertIn("route-proofs/floor-01.vtr", manifest["files"])
            proof = output / "route-proofs/floor-01.json"
            self.assertEqual(load_route_proof(
                proof, (output / "floors/floor-01.vwi").read_bytes(),
                (output / "floors/floor-01-entities.vwe").read_bytes())['version'], 3)

    def test_campaign_rejects_truncated_loot_table(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            base=root/"base-act-one";base.mkdir()
            self.make_package(base,"act-one-world.vwi",b"VWI2-base")
            loot=b"VLT1-truncated"
            (base/"item-loot.vlt").write_bytes(loot)
            manifest=json.loads((base/"manifest.json").read_text("utf-8"))
            manifest["files"]["item-loot.vlt"]={"bytes":len(loot),"sha256":hashlib.sha256(loot).hexdigest()}
            (base/"manifest.json").write_text(json.dumps(manifest),encoding="utf-8")
            floor1=self.make_floor_export(root,"export-1")
            floor2=self.make_floor_export(root,"export-2")
            with self.assertRaisesRegex(ValueError,"invalid VLT1 v2 size or header"):
                prepare_campaign(base,root/"campaign",[(1,floor1),(2,floor2)])

    def test_campaign_requires_both_unique_later_floor_indices(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            base = root / "base-act-one"
            base.mkdir()
            self.make_package(base, "act-one-world.vwi", b"VWI2-base")
            floor1 = self.make_floor_export(root, "export-1")
            with self.assertRaisesRegex(ValueError, "floor 1 and floor 2"):
                prepare_campaign(base, root / "campaign", [(1, floor1)])

    def test_campaign_rejects_incomplete_floor_geometry_report(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            base = root / "base-act-one"
            base.mkdir()
            self.make_package(base, "act-one-world.vwi", b"VWI2-base")
            floor1 = self.make_floor_export(root, "export-1")
            floor2 = self.make_floor_export(root, "export-2")
            (floor2 / "act-one-export.json").write_text(json.dumps({
                "floor_complete": False, "native_tile_path": {"reachable": True},
                "native_player_spawn_geometry": {"verified": False},
            }), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "spawn or tile-route"):
                prepare_campaign(base, root / "campaign", [(1, floor1), (2, floor2)])


if __name__=="__main__":unittest.main()
