"""Render private native tile-probe output using the user's extracted atlases."""
import argparse,csv,json
from pathlib import Path
from PIL import Image


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("tiles",type=Path)
    p.add_argument("assets",type=Path)
    p.add_argument("output",type=Path)
    args=p.parse_args()
    if args.output.exists():raise FileExistsError(args.output)
    rows=list(csv.DictReader(args.tiles.open(encoding="utf-8")))
    if len(rows)!=960:raise ValueError("Expected all 960 tiles")
    image=Image.new("RGBA",(640,384),(16,24,32,255))
    names=["background","farground","midground","foreground","closeground"]
    report={"cells":960,"layers":{},"lighting_implemented":False,"doodads_implemented":False}
    for name in names:
        atlas=Image.open(args.assets/"gfx"/f"tiles-{name}.png").convert("RGBA")
        columns=atlas.width//16;limit=columns*(atlas.height//16);drawn=0;skipped=0
        for row in rows:
            index=int(row[name])
            if index<0:skipped+=1;continue
            if index>=limit:raise ValueError(f"{name} index {index} outside atlas")
            x=int(row["x"]);y=int(row["y"])
            image.alpha_composite(atlas.crop((index%columns*16,index//columns*16,index%columns*16+16,index//columns*16+16)),(x*16,y*16))
            drawn+=1
        report["layers"][name]={"drawn":drawn,"negative_skipped":skipped}
    image.convert("RGB").save(args.output)
    with args.output.with_suffix(".json").open("x",encoding="utf-8") as f:json.dump(report,f,indent=2)
    print(args.output)


if __name__=="__main__":main()
