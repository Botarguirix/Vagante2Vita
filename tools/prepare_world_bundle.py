"""Prepare private runtime resources; commercial images are never committed."""
import argparse,hashlib,json,shutil
from pathlib import Path


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("assets",type=Path)
    p.add_argument("packet",type=Path)
    p.add_argument("terrain",type=Path)
    p.add_argument("output",type=Path)
    args=p.parse_args()
    sources={"intro-world.vwi":args.packet,"intro-terrain.png":args.terrain}
    for cls in ("knight","rogue","mage"):
        for anim in ("stand","walk","jump","fall","duck","climb","lookup","hang"):
            sources[f"{cls}/{anim}.png"]=args.assets/"gfx"/"classes"/cls/"1"/(anim+".png")
    for src in sources.values():
        if not src.is_file():raise FileNotFoundError(src)
    args.output.mkdir(parents=True,exist_ok=False)
    manifest={}
    for name,src in sources.items():
        dst=args.output/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
        manifest[name]={"bytes":dst.stat().st_size,"sha256":hashlib.sha256(dst.read_bytes()).hexdigest()}
    (args.output/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print(args.output)


if __name__=="__main__":main()
