"""Verify and stage the private intro-world resource bundle."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path, PurePosixPath


REQUIRED = {
    'intro-world.vwi', 'intro-entities.vwe', 'intro-terrain.png',
    'intro-melee.vwa', 'intro-bow.vwb',
}


def stage(source: Path, destination: Path) -> int:
    source = source.resolve()
    destination = destination.resolve()
    manifest_path = source / 'manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    entries = manifest.get('files') if isinstance(manifest.get('files'), dict) else manifest
    if not isinstance(entries, dict) or not REQUIRED.issubset(entries):
        raise ValueError('Intro bundle manifest is missing required world resources')

    verified = []
    for name, record in entries.items():
        rel = PurePosixPath(name)
        if (not name or rel.is_absolute() or '..' in rel.parts or '\\' in name
                or not isinstance(record, dict)):
            raise ValueError(f'Unsafe intro bundle path: {name}')
        file = (source / Path(*rel.parts)).resolve()
        if not file.is_relative_to(source) or not file.is_file():
            raise ValueError(f'Missing or unsafe intro bundle file: {name}')
        data = file.read_bytes()
        if record.get('bytes') != len(data) or record.get('sha256') != hashlib.sha256(data).hexdigest():
            raise ValueError(f'Intro bundle hash mismatch: {name}')
        verified.append((name, file))

    destination.mkdir(parents=True, exist_ok=True)
    for name, source_file in verified:
        target = destination.joinpath(*PurePosixPath(name).parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_file, target)
    return len(verified)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    count = stage(args.source, args.destination)
    print(f'STAGED intro_bundle files={count}')


if __name__ == '__main__':
    main()
