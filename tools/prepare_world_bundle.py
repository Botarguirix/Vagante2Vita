"""Prepare private runtime resources; commercial images are never committed."""
import argparse,hashlib,json,shutil,struct
from pathlib import Path

FORMAT="vagante-vita-world"
FORMAT_VERSION=1

def validate_manifest(root:Path, manifest_path:Path|None=None):
    """Validate a private external world pack and every file it declares."""
    root=root.resolve()
    path=(manifest_path or root/"manifest.json").resolve()
    if not path.is_relative_to(root):raise ValueError("Manifest must be inside package root")
    doc=json.loads(path.read_text(encoding="utf-8"))
    if doc.get("format")!=FORMAT or doc.get("version")!=FORMAT_VERSION:
        raise ValueError("Unsupported world package format/version")
    files=doc.get("files")
    if not isinstance(files,dict) or not files:raise ValueError("Manifest has no files")
    seen=set()
    for name,record in files.items():
        rel=Path(name)
        if rel.is_absolute() or ".." in rel.parts or "\\" in name or not name or name in seen:
            raise ValueError(f"Unsafe or duplicate package path: {name}")
        seen.add(name);file=(root/rel).resolve()
        if not file.is_relative_to(root) or not file.is_file():raise ValueError(f"Missing or unsafe package file: {name}")
        data=file.read_bytes()
        if record.get("bytes")!=len(data) or record.get("sha256")!=hashlib.sha256(data).hexdigest():
            raise ValueError(f"Size/hash mismatch: {name}")
    return doc


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("assets",type=Path,nargs="?")
    p.add_argument("packet",type=Path,nargs="?")
    p.add_argument("terrain",type=Path,nargs="?")
    p.add_argument("output",type=Path,nargs="?")
    p.add_argument("--intro-metadata",type=Path,help="Private original INTRO metadata including entities")
    p.add_argument("--attacks",type=Path,help="Private original attacks.json for base melee")
    p.add_argument("--act-one",action="store_true",help="Name the world packet act-one-world.vwi for the ACT_ONE runtime directory")
    p.add_argument("--entity-packet",type=Path,help="Private VWE2 ACT_ONE entity packet from export_act_one_world.py")
    p.add_argument("--act-one-report",type=Path,help="Private act-one-export.json report to include in the ACT_ONE bundle")
    p.add_argument("--loot-tables",type=Path,help="Verified VLT1 version 2 item-loot table package")
    p.add_argument("--source-archive",type=Path,
                   help="Private source data.vra; record its SHA-256 without copying the archive")
    p.add_argument("--validate",type=Path,help="Validate an existing external world package; no extraction or copying")
    args=p.parse_args()
    if args.validate:
        doc=validate_manifest(args.validate)
        print(json.dumps({"valid":True,"format":doc["format"],"version":doc["version"],"files":len(doc["files"])},indent=2));return
    if not all((args.assets,args.packet,args.terrain,args.output)):
        p.error("assets, packet, terrain and output are required when preparing a package")
    packet_name="act-one-world.vwi" if args.act_one else "intro-world.vwi"
    sources={packet_name:args.packet,"intro-terrain.png":args.terrain}
    if args.entity_packet:
        if not args.act_one:raise ValueError("--entity-packet requires --act-one")
        header=args.entity_packet.read_bytes()[:8]
        if len(header)!=8 or header[:4]!=b"VWE2" or struct.unpack_from("<H",header,6)[0]!=16:
            raise ValueError("ACT_ONE entity packet must use VWE2 16-byte records")
        sources["act-one-entities.vwe"]=args.entity_packet
        enemy_packet=args.entity_packet.with_name("act-one-enemies.vwe")
        if enemy_packet.is_file():
            header=enemy_packet.read_bytes()[:8]
            if len(header)!=8 or header[:4]!=b"VWE3" or struct.unpack_from("<H",header,6)[0]!=16:
                raise ValueError("ACT_ONE enemy packet must use VWE3 16-byte records")
            sources["act-one-enemies.vwe"]=enemy_packet
        layout_file=args.entity_packet.with_name("act-one-layout.json")
        if layout_file.is_file():sources["act-one-layout.json"]=layout_file
    if args.act_one_report:
        if not args.act_one:raise ValueError("--act-one-report requires --act-one")
        report=json.loads(args.act_one_report.read_text(encoding="utf-8"))
        required=("exact_generation_parity","entities_instantiated",
                  "native_player_route_verified","native_exit_transition_verified")
        declared=report.get("floor_complete") is True
        if declared and not all(report.get(key) is True for key in required):
            raise ValueError("ACT_ONE report claims completion without all required evidence")
        report["floor_complete"]=declared
        # Normalize older export reports so the installed bundle exposes every
        # completion gate explicitly instead of relying on absent JSON keys.
        for key in required:
            report[key]=report.get(key) is True
        sources.pop("act-one-export.json",None)
    if args.act_one:
        sources.update({
            "entities/chest.png":args.assets/"gfx/chest_small.png",
            # world_visuals loads these three sheets from the flat entities/
            # directory; the doodad atlas copies below serve tile/entity art.
            "entities/torch.png":args.assets/"gfx/doodads/torch.png",
            "entities/flower_1.png":args.assets/"gfx/doodads/flower_1.png",
            "entities/little_firefly.png":args.assets/"gfx/scenes/intro/entities/little_firefly.png",
            "entities/breakable_wall.png":args.assets/"gfx/doodads/breakable_wall.png",
            "entities/spikes.png":args.assets/"gfx/traps/spikes.png",
            "entities/spikes_broken.png":args.assets/"gfx/traps/spikes_broken.png",
            "entities/arrow_trap.png":args.assets/"gfx/traps/arrow_trap.png",
            "entities/trap_trigger.png":args.assets/"gfx/traps/trap_trigger.png",
            "entities/boulder_vine.png":args.assets/"gfx/traps/boulder_vine.png",
            "entities/boulder_blocker.png":args.assets/"gfx/traps/boulder_blocker.png",
            "entities/boulders.png":args.assets/"gfx/traps/boulders.png",
            "entities/shake_boulder.png":args.assets/"gfx/traps/shake_boulder.png",
        })
        # Ship original enemy sheets and the Goblin attack pose in the private
        # bundle, never in the VPK. Repeated sheets have stable state names.
        enemy_sprites={
            "goblin":("goblin_stand.png","goblin_walk.png","goblin_jump.png","goblin_hurt.png","goblin_attack.png"),
            "bat":("bat_hang.png","bat.png","bat.png","bat_hurt.png","bat.png"),
            "lurker":("lurker_retract.png","lurker_attack.png","lurker_retract.png","lurker_retract.png","lurker_attack.png"),
            "slime":("stand.png","walk.png","jump.png","jump.png","walk.png"),
        }
        for family,filenames in enemy_sprites.items():
            for state,filename in zip(("stand","move","air","hurt","attack"),filenames):
                sources[f"enemies/{family}/{state}.png"]=args.assets/"gfx"/"monster"/family/filename
        doodads={1:"counter.png",2:"shop_sign.png",3:"torch.png",4:"decorative_helmet.png",
                 5:"decorative_shield.png",13:"flower_1.png",14:"spellbook_altar.png",
                 19:"waterfall_1_2.png",20:"waterfall_1_3.png",21:"waterfall_1_4.png"}
        for name in doodads.values():sources[f"entities/doodads/{name}"]=args.assets/"gfx/doodads"/name
    for cls in ("knight","rogue","mage"):
        for anim in ("stand","walk","jump","fall","duck","climb","lookup","hang"):
            sources[f"{cls}/{anim}.png"]=args.assets/"gfx"/"classes"/cls/"1"/(anim+".png")
    if args.intro_metadata:
        for name in ("torch","flower_1"):
            sources[f"entities/{name}.png"]=args.assets/"gfx"/"doodads"/(name+".png")
        sources["entities/little_firefly.png"]=args.assets/"gfx/scenes/intro/entities/little_firefly.png"
    if args.attacks:
        sources["items-weapons.png"]=args.assets/"gfx/items/items-weapons.png"
        for cls in ("knight","rogue","mage"):
            sources[f"{cls}/stab.png"]=args.assets/"gfx/classes"/cls/"1/stab.png"
    if args.attacks:
        for name in ("aim","aim_arm"):
            sources[f"rogue/{name}.png"]=args.assets/"gfx/classes/rogue/1"/(name+".png")
        sources["entities/dart.png"]=args.assets/"gfx/projectiles/dart.png"
    if args.loot_tables:
        if not args.act_one:raise ValueError("--loot-tables requires --act-one")
        loot=args.loot_tables.read_bytes()
        if len(loot)<14 or loot[:4]!=b"VLT1" or struct.unpack_from("<H",loot,4)[0]!=2:
            raise ValueError("loot table package must use VLT1 version 2")
        sources["item-loot.vlt"]=args.loot_tables
        sources["items-weapons.png"]=args.assets/"gfx/items/items-weapons.png"
        for name in ("items.png","items-weapons.png","items-armor.png","potions.png",
                     "scrolls.png","spell_books.png","items-explosives.png"):
            sources[f"items/{name}"]=args.assets/"gfx/items"/name
    for src in sources.values():
        if not src.is_file():raise FileNotFoundError(src)
    args.output.mkdir(parents=True,exist_ok=False)
    manifest={}
    for name,src in sources.items():
        dst=args.output/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
        manifest[name]={"bytes":dst.stat().st_size,"sha256":hashlib.sha256(dst.read_bytes()).hexdigest()}
    if args.act_one_report:
        report_path=args.output/"act-one-export.json"
        report_path.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
        data=report_path.read_bytes()
        manifest["act-one-export.json"]={"bytes":len(data),"sha256":hashlib.sha256(data).hexdigest()}
    if args.intro_metadata:
        meta=json.loads(args.intro_metadata.read_bytes());entities=meta["entities"]
        if len(entities)>64:raise ValueError("Too many intro entities")
        packet=bytearray(struct.pack("<4sHH",b"VWE1",len(entities),0))
        for e in entities:
            kind=1 if e["type"]=="Doodad" else 2 if e["type"]=="Firefly" else 0
            subtype=e.get("doodadType",0)
            if not kind or (kind==1 and subtype not in (3,11,12,13,15)):raise ValueError("Unsupported entity")
            if not 0<=e["x"]<640 or not 0<=e["y"]<384:raise ValueError("Invalid coordinates")
            packet.extend(struct.pack("<BBhhH",kind,subtype,e["x"],e["y"],0))
        (args.output/"intro-entities.vwe").write_bytes(packet)
        manifest["intro-entities.vwe"]={"bytes":len(packet),"sha256":hashlib.sha256(packet).hexdigest()}
    if args.attacks:
        from prepare_melee import encode
        packet=encode(args.attacks)
        (args.output/"intro-melee.vwa").write_bytes(packet)
        manifest["intro-melee.vwa"]={"bytes":len(packet),"sha256":hashlib.sha256(packet).hexdigest(),"source_sha256":hashlib.sha256(args.attacks.read_bytes()).hexdigest()}
        if args.act_one:
            from prepare_goblin_attack import encode as encode_goblin_attack
            goblin_packet=encode_goblin_attack(args.attacks)
            (args.output/"act-one-goblin-attack.vga").write_bytes(goblin_packet)
            manifest["act-one-goblin-attack.vga"]={"bytes":len(goblin_packet),"sha256":hashlib.sha256(goblin_packet).hexdigest(),"source_sha256":hashlib.sha256(args.attacks.read_bytes()).hexdigest()}
    if args.attacks:
        from prepare_bow import encode as encode_bow
        packet=encode_bow(args.attacks);(args.output/"intro-bow.vwb").write_bytes(packet)
        manifest["intro-bow.vwb"]={"bytes":len(packet),"sha256":hashlib.sha256(packet).hexdigest()}
    document={"format":FORMAT,"version":FORMAT_VERSION,"files":manifest}
    if args.source_archive:
        if not args.source_archive.is_file():
            raise FileNotFoundError(args.source_archive)
        archive_bytes=args.source_archive.read_bytes()
        document["source"]={
            "kind":"vagante-data-vra",
            "bytes":len(archive_bytes),
            "sha256":hashlib.sha256(archive_bytes).hexdigest(),
            "included":False,
        }
    (args.output/"manifest.json").write_text(json.dumps(document,indent=2)+"\n",encoding="utf-8")
    validate_manifest(args.output)
    print(args.output)


if __name__=="__main__":main()
