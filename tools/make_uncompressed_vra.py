"""Write an uncompressed copy of data.vra for the PS Vita build (same records, no zlib layer).

    python tools/make_uncompressed_vra.py <vagante>/data.vra _local/vita-data/data.vra

The port's ResourceArchive accepts both forms. Copy the output to ux0:data/Vagante/data.vra.
It is ~119 MB instead of ~95 MB, but the Vita reads it in a few seconds instead of inflating
it for ~20 s. The file stays the user's own game data: never commit or redistribute it.
"""
import argparse, pathlib, zlib

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('source', type=pathlib.Path)
    ap.add_argument('output', type=pathlib.Path)
    a = ap.parse_args()
    data = zlib.decompress(a.source.read_bytes())
    if data[:1] == b'\x78':
        raise SystemExit('unexpected: decompressed data starts with a zlib header byte')
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(data)
    print('%s: %d bytes (from %d compressed)' % (a.output, len(data), a.source.stat().st_size))

if __name__ == '__main__':
    main()
