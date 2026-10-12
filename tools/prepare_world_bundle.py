"""Prepare private runtime resources; commercial images are never committed."""
import argparse,hashlib,json,shutil,struct
from pathlib import Path


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("assets",type=Path)
    p.add_argument("packet",type=Path)
    p.add_argument("terrain",type=Path)
    p.add_argument("output",type=Path)
    p.add_argument("--intro-metadata",type=Path,help="Private original INTRO metadata including entities")
    p.add_argument("--attacks",type=Path,help="Private original attacks.json for base melee")
    args=p.parse_args()
    sources={"intro-world.vwi":args.packet,"intro-terrain.png":args.terrain}
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
    for src in sources.values():
        if not src.is_file():raise FileNotFoundError(src)
    args.output.mkdir(parents=True,exist_ok=False)
    manifest={}
    for name,src in sources.items():
        dst=args.output/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
        manifest[name]={"bytes":dst.stat().st_size,"sha256":hashlib.sha256(dst.read_bytes()).hexdigest()}
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
    if args.attacks:
        from prepare_bow import encode as encode_bow
        packet=encode_bow(args.attacks);(args.output/"intro-bow.vwb").write_bytes(packet)
        manifest["intro-bow.vwb"]={"bytes":len(packet),"sha256":hashlib.sha256(packet).hexdigest()}
    (args.output/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print(args.output)


if __name__=="__main__":main()
