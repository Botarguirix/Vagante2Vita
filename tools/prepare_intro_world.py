"""Build a private, bounded tile packet from the user's original intro room."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_world_rooms import audit


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("rooms",type=Path)
    p.add_argument("output",type=Path)
    args=p.parse_args()
    audit(args.rooms)
    data=json.loads(args.rooms.read_bytes())
    rooms=data["acts"][9]["rooms"]["4"]
    if len(rooms)!=1:raise ValueError("Multiple INTRO variants require verified selection logic")
    room=rooms[0]
    packet=bytearray(struct.pack("<4sHHII",b"VWI1",40,24,0,960))
    for row in room["tiles"]:
        for x in range(40):
            tile=row[x*6:(x+1)*6]
            if not 0<=tile[0]<=12 or any(not -32768<=v<=32767 for v in tile[1:]):
                raise ValueError("Unsupported tile fields")
            packet.extend(struct.pack("<bB5h",tile[0],0,*tile[1:]))
    if args.output.exists() or args.output.with_suffix(".json").exists():raise FileExistsError(args.output)
    with args.output.open("xb") as f:f.write(packet)
    report={"source_sha256":hashlib.sha256(args.rooms.read_bytes()).hexdigest(),
            "packet_sha256":hashlib.sha256(packet).hexdigest(),"bytes":len(packet),
            "act":9,"group":4,"width":40,"height":24,"entities":room["entities"],
            "fields":["type","backgroundIndex","fargroundIndex","midgroundIndex","foregroundIndex","closegroundIndex"],
            "notes":"Raw room tiles; no autotiling, lighting, doodad runtime or complete physics yet."}
    with args.output.with_suffix(".json").open("x",encoding="utf-8") as f:json.dump(report,f,indent=2)
    print(args.output)


if __name__=="__main__":main()
