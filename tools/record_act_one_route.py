"""Turn a successful native route-probe log into a hash-bound proof record."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

try:
    from .route_probe_fingerprint import source_fingerprint
except ImportError:
    from route_probe_fingerprint import source_fingerprint

try:
    from .route_trace import read_trace
except ImportError:
    from route_trace import read_trace


def record(log_path: Path, world: Path, entities: Path, output: Path, seed: int,
           source_root: Path | None = None, trace_path: Path | None = None):
    log = log_path.read_text(encoding='utf-8', errors='replace')
    match = re.search(
        r'ACT_ONE_ROUTE seed=(\d+) tiles=(\d+) waypoint=(\d+) ticks=(\d+) '
        r'jumps=(\d+) reached_exit=(\d+)', log)
    if not match:
        raise ValueError('Probe log has no ACT_ONE_ROUTE result')
    logged_seed, tiles, waypoint, ticks, jumps, reached = map(int, match.groups())
    if logged_seed != seed or reached != 1 or not tiles or waypoint < tiles - 1:
        raise ValueError('Route probe did not reach the requested floor exit')
    if source_root is None:
        source_root = Path(__file__).resolve().parents[1]
    trace_masks = read_trace(trace_path) if trace_path else None
    if trace_masks is not None and len(trace_masks) != ticks:
        raise ValueError('Controller trace frame count does not match the successful route log')
    report = dict(format='act-one-native-route-proof', version=3 if trace_path else 2, seed=seed,
                  reached_exit=True, route_tiles=tiles, final_waypoint=waypoint,
                  ticks=ticks, jumps=jumps,
                  world_sha256=hashlib.sha256(world.read_bytes()).hexdigest(),
                  entities_sha256=hashlib.sha256(entities.read_bytes()).hexdigest(),
                  simulator=source_fingerprint(source_root))
    output.parent.mkdir(parents=True, exist_ok=True)
    if trace_path:
        trace_output = output.with_suffix('.vtr')
        if trace_path.resolve() != trace_output.resolve():
            shutil.copyfile(trace_path, trace_output)
        report.update(controller_trace_file=trace_output.name,
                      controller_trace_frames=len(trace_masks),
                      controller_trace_sha256=hashlib.sha256(trace_output.read_bytes()).hexdigest())
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--world', type=Path, required=True)
    parser.add_argument('--entities', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, required=True)
    parser.add_argument('--trace', type=Path,
                        help='successful VTR1 input trace produced with --trace-out by the C route probe')
    args = parser.parse_args()
    print(json.dumps(record(args.log, args.world, args.entities,
                            args.output, args.seed, trace_path=args.trace), indent=2))


if __name__ == '__main__':
    main()
