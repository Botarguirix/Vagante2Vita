"""Extend a private ACT_ONE floor-0 pack with additional VWI2 floors."""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

try:
    from .route_probe_fingerprint import source_fingerprint
    from .route_trace import read_trace
except ImportError:
    from route_probe_fingerprint import source_fingerprint
    from route_trace import read_trace

try:
    from tools.prepare_world_bundle import FORMAT, FORMAT_VERSION, validate_manifest
except ModuleNotFoundError:
    from prepare_world_bundle import FORMAT, FORMAT_VERSION, validate_manifest


def load_route_proof(proof_path: Path | None, world: bytes, entities: bytes,
                     source_root: Path | None = None, enemies: bytes | None = None):
    if proof_path is None:
        return None
    proof = json.loads(proof_path.read_text('utf-8'))
    if (proof.get('format') != 'act-one-native-route-proof'
            or proof.get('version') not in (1, 2, 3, 4, 5) or proof.get('reached_exit') is not True
            or proof.get('world_sha256') != hashlib.sha256(world).hexdigest()
            or proof.get('entities_sha256') != hashlib.sha256(entities).hexdigest()):
        raise ValueError(f'{proof_path}: route proof is invalid or belongs to different floor data')
    if proof.get('version') == 5 and proof.get('player_class') not in (0, 1, 2):
        raise ValueError(f'{proof_path}: class-tagged route proof has no valid player class')
    if 'boss' in proof:
        if (proof.get('version') not in (4, 5) or enemies is None
                or proof.get('enemies_sha256') != hashlib.sha256(enemies).hexdigest()):
            raise ValueError(f'{proof_path}: boss proof is not bound to this floor enemy packet')
    elif proof.get('enemies_sha256') is not None:
        if enemies is None or proof['enemies_sha256'] != hashlib.sha256(enemies).hexdigest():
            raise ValueError(f'{proof_path}: route proof belongs to different floor enemy data')
    if proof['version'] in (2, 3, 4, 5):
        if source_root is None:
            source_root = Path(__file__).resolve().parents[1]
        if proof.get('simulator') != source_fingerprint(source_root):
            raise ValueError(f'{proof_path}: route proof was produced by different simulator sources')
    if proof['version'] in (3, 4, 5) and 'controller_trace_file' in proof:
        trace_name = proof.get('controller_trace_file')
        trace_path = proof_path.with_suffix('.vtr')
        if trace_name != trace_path.name or not trace_path.is_file():
            raise ValueError(f'{proof_path}: controller trace sidecar is missing or unsafe')
        trace = read_trace(trace_path)
        if (len(trace) != proof.get('controller_trace_frames')
                or hashlib.sha256(trace_path.read_bytes()).hexdigest() != proof.get('controller_trace_sha256')):
            raise ValueError(f'{proof_path}: controller trace does not match its proof')
    return proof


def validate_campaign_route_classes(proofs):
    """Reject class-tagged route sets that cannot describe one campaign run."""
    proofs = list(proofs)
    if not any(proof and proof.get('version') == 5 for proof in proofs):
        return None
    if (len(proofs) != 3 or any(proof is None or proof.get('version') != 5 for proof in proofs)
            or any(proof.get('player_class') not in (0, 1, 2) for proof in proofs)):
        raise ValueError('Class-tagged ACT_ONE campaign proofs must cover all floors with one player class')
    classes = {proof['player_class'] for proof in proofs}
    if len(classes) != 1:
        raise ValueError('Class-tagged ACT_ONE campaign proofs must cover all floors with one player class')
    return classes.pop()


def validate_floor(export_dir: Path, route_proof_path: Path | None = None):
    export_dir = export_dir.resolve()
    world = (export_dir / "act-one-world.vwi").read_bytes()
    if len(world) < 32 or world[:4] != b"VWI2":
        raise ValueError(f"{export_dir}: floor world must use VWI2")
    width, height, flags, count = struct.unpack_from("<HHII", world, 4)
    if not width or not height or width > 128 or height > 128 or count != width * height:
        raise ValueError(f"{export_dir}: invalid VWI2 dimensions")
    if flags & ~7 or len(world) != 32 + count * 12:
        raise ValueError(f"{export_dir}: invalid VWI2 payload size")
    entities = (export_dir / "act-one-entities.vwe").read_bytes()
    if len(entities) < 8 or entities[:4] != b"VWE2" or struct.unpack_from("<H", entities, 6)[0] != 16:
        raise ValueError(f"{export_dir}: entities must use VWE2")
    if len(entities) != 8 + struct.unpack_from("<H", entities, 4)[0] * 16:
        raise ValueError(f"{export_dir}: invalid VWE2 payload size")
    enemies = (export_dir / "act-one-enemies.vwe").read_bytes()
    if len(enemies) < 8 or enemies[:4] != b"VWE3" or struct.unpack_from("<H", enemies, 6)[0] != 16:
        raise ValueError(f"{export_dir}: enemies must use VWE3")
    if len(enemies) != 8 + struct.unpack_from("<H", enemies, 4)[0] * 16:
        raise ValueError(f"{export_dir}: invalid VWE3 payload size")
    terrain = export_dir / "intro-terrain.png"
    layout = json.loads((export_dir / "act-one-layout.json").read_text("utf-8"))
    summary = json.loads((export_dir / "act-one-export.json").read_text("utf-8"))
    if not terrain.is_file() or summary.get("floor_complete") is not False:
        raise ValueError(f"{export_dir}: missing terrain or invalid completion claim")
    geometry = summary.get("native_player_spawn_geometry", {})
    path = summary.get("native_tile_path", {})
    if geometry.get("verified") is not True or path.get("reachable") is not True:
        raise ValueError(f"{export_dir}: spawn or tile-route audit is incomplete")
    route_proof = load_route_proof(route_proof_path, world, entities, enemies=enemies)
    return dict(width=width, height=height, entities=struct.unpack_from("<H", entities, 4)[0],
                provisional_enemies=struct.unpack_from("<H", enemies, 4)[0],
                exact_generation_parity=summary.get("exact_generation_parity") is True,
                native_player_route_verified=route_proof is not None,
                native_exit_transition_verified=False,
                floor_complete=False,
                native_route_proof=route_proof,
                layout_sha256=hashlib.sha256((export_dir / "act-one-layout.json").read_bytes()).hexdigest())


def validate_loot_table(path: Path):
    """Validate the complete VLT1 v2 packet before carrying it into a campaign."""
    data = path.read_bytes()
    header = struct.Struct("<4sHHHHH")
    descriptor = struct.Struct("<BBHH")
    rarity = struct.Struct("<IIHHBBH")
    edge = struct.Struct("<BBI")
    if len(data) < header.size:
        raise ValueError(f"{path}: truncated VLT1 header")
    magic, version, direct_count, row_count, nested_count, edge_count = header.unpack_from(data)
    expected = header.size + (direct_count + nested_count) * descriptor.size + row_count * rarity.size + edge_count * edge.size
    if magic != b"VLT1" or version != 2 or len(data) != expected:
        raise ValueError(f"{path}: invalid VLT1 v2 size or header")
    offset = header.size
    direct = []
    for _ in range(direct_count):
        pool_id, reserved, first, count = descriptor.unpack_from(data, offset)
        offset += descriptor.size
        if reserved or pool_id >= 26 or not count or first + count > row_count or any(item[0] == pool_id for item in direct):
            raise ValueError(f"{path}: invalid VLT1 direct pool descriptor")
        direct.append((pool_id, first, count))
    for _ in range(row_count):
        minimum, maximum, weight, item_type, flags, atlas, sprite = rarity.unpack_from(data, offset)
        offset += rarity.size
        if minimum > maximum or maximum > 99999 or not weight or flags & ~1 or atlas > 6:
            raise ValueError(f"{path}: invalid VLT1 rarity row")
    nested = []
    for _ in range(nested_count):
        pool_id, reserved, first, count = descriptor.unpack_from(data, offset)
        offset += descriptor.size
        if reserved or pool_id >= 26 or not count or first + count > edge_count or any(item[0] == pool_id for item in nested):
            raise ValueError(f"{path}: invalid VLT1 nested pool descriptor")
        nested.append((pool_id, first, count))
    children = []
    for _ in range(edge_count):
        child, reserved, weight = edge.unpack_from(data, offset)
        offset += edge.size
        if reserved or child >= 26 or not weight:
            raise ValueError(f"{path}: invalid VLT1 nested pool edge")
        children.append(child)
    pool_ids = {pool_id for pool_id, _, _ in direct + nested}
    if (not direct_count or direct_count > 26 or row_count > 256 or not nested_count
            or nested_count > 26 or edge_count > 64
            or not any(pool_id == 23 for pool_id, _, _ in nested)):
        raise ValueError(f"{path}: VLT1 packet has no chest loot root")
    if any(child not in pool_ids for child in children):
        raise ValueError(f"{path}: VLT1 edge refers to an undefined pool")
    return dict(version=version, direct_pools=direct_count, rarity_rows=row_count,
                nested_pools=nested_count, edges=edge_count,
                sha256=hashlib.sha256(data).hexdigest())


def prepare(base: Path, output: Path, floors, route_proofs=None):
    base, output = base.resolve(), output.resolve()
    validate_manifest(base)
    loot_path = base / 'item-loot.vlt'
    if output.exists():
        raise FileExistsError(output)
    checked = {}
    route_proofs = route_proofs or {}
    base_proof = None
    if 0 in route_proofs:
        base_world = (base / 'act-one-world.vwi').read_bytes()
        base_entities = (base / 'act-one-entities.vwe').read_bytes()
        base_enemies = (base / 'act-one-enemies.vwe').read_bytes()
        base_proof = load_route_proof(route_proofs[0], base_world, base_entities,
                                      enemies=base_enemies)
    for index, source in floors:
        if index not in (1, 2) or index in checked:
            raise ValueError("Campaign extension requires unique floor indices 1 and 2")
        checked[index] = (source.resolve(), validate_floor(source, route_proofs.get(index)))
    if set(checked) != {1, 2}:
        raise ValueError("Campaign extension requires floor 1 and floor 2")
    if set(route_proofs) - {0, 1, 2}:
        raise ValueError('Route proof index must be 0, 1, or 2')
    campaign_proofs = [base_proof] + [checked[index][1]['native_route_proof'] for index in (1, 2)]
    validate_campaign_route_classes(campaign_proofs)
    loot_report = validate_loot_table(loot_path) if loot_path.is_file() else None
    if loot_report is None:
        raise ValueError('ACT_ONE campaign requires a verified item-loot.vlt VLT1 v2 table')
    shutil.copytree(base, output)
    root = output
    manifest_path = root / "manifest.json"
    manifest = json.loads(manifest_path.read_text("utf-8"))
    if manifest.get("format") != FORMAT or manifest.get("version") != FORMAT_VERSION:
        shutil.rmtree(output)
        raise ValueError("Base ACT_ONE package has an unsupported manifest")
    files = manifest["files"]
    for index, proof_path in route_proofs.items():
        destination = root / f'route-proofs/floor-{index:02d}.json'
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(proof_path, destination)
        if json.loads(destination.read_text('utf-8')).get('version') in (3, 4, 5):
            source_trace = proof_path.with_suffix('.vtr')
            destination_trace = destination.with_suffix('.vtr')
            shutil.copyfile(source_trace, destination_trace)
            proof_doc = json.loads(destination.read_text('utf-8'))
            proof_doc['controller_trace_file'] = destination_trace.name
            destination.write_text(json.dumps(proof_doc, indent=2) + '\n', encoding='utf-8')
            trace_data = destination_trace.read_bytes()
            files[destination_trace.relative_to(root).as_posix()] = {
                "bytes": len(trace_data), "sha256": hashlib.sha256(trace_data).hexdigest()}
        data = destination.read_bytes()
        files[destination.relative_to(root).as_posix()] = {
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    campaign = dict(format="vagante-vita-act-one-campaign", version=1, seed=12345,
                    loot_table=loot_report,
                    floors={"0": {"source": "existing-floor-0", "floor_complete": False,
                                   "native_player_route_verified": base_proof is not None,
                                   "native_exit_transition_verified": False,
                                   "native_route_proof": base_proof}},
                    campaign_complete=False,
                    caveat="Floors 1-2 use provisional route geometry repairs; exact generation parity remains unverified, the runtime next-floor transition is implemented but still needs a successful Vita3K replay.")
    for index, (source, report) in sorted(checked.items()):
        folder = root / "floors"
        folder.mkdir(exist_ok=True)
        stem = f"floor-{index:02d}"
        mapping = {
            f"{stem}.vwi": "act-one-world.vwi",
            f"{stem}-entities.vwe": "act-one-entities.vwe",
            f"{stem}-enemies.vwe": "act-one-enemies.vwe",
            f"{stem}-terrain.png": "intro-terrain.png",
            f"{stem}-layout.json": "act-one-layout.json",
        }
        for destination, filename in mapping.items():
            src = source / filename
            dst = folder / destination
            shutil.copyfile(src, dst)
            data = dst.read_bytes()
            relative = f"floors/{destination}"
            files[relative] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
        campaign["floors"][str(index)] = report
    campaign_path = root / "campaign.json"
    campaign_path.write_text(json.dumps(campaign, indent=2) + "\n", encoding="utf-8")
    data = campaign_path.read_bytes()
    files["campaign.json"] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    validate_manifest(root)
    return root


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path, help="Private ACT_ONE package directory with manifest.json")
    parser.add_argument("output", type=Path)
    parser.add_argument("--floor", action="append", required=True, metavar="INDEX=EXPORT_DIR")
    parser.add_argument("--route-proof", action="append", default=[], metavar="INDEX=JSON")
    args = parser.parse_args()
    floors = []
    for spec in args.floor:
        index, separator, path = spec.partition("=")
        if not separator:
            parser.error("--floor must use INDEX=EXPORT_DIR")
        floors.append((int(index), Path(path)))
    route_proofs = {}
    for spec in args.route_proof:
        index, separator, path = spec.partition('=')
        if not separator:
            parser.error('--route-proof must use INDEX=JSON')
        key = int(index)
        if key in route_proofs:
            parser.error('route proof indices must be unique')
        route_proofs[key] = Path(path).resolve()
    print(prepare(args.base, args.output, floors, route_proofs))


if __name__ == "__main__":
    main()
