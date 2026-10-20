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
           source_root: Path | None = None, trace_path: Path | None = None,
           require_boss_defeat: bool = False,
           simulator_files: list[str] | None = None,
           simulator_sha256: str | None = None,
           enemies: Path | None = None):
    log = log_path.read_text(encoding='utf-8', errors='replace')
    match = re.search(
        r'ACT_ONE_ROUTE seed=(\d+) tiles=(\d+) waypoint=(\d+) ticks=(\d+) '
        r'jumps=(\d+) reached_exit=(\d+)', log)
    if not match:
        raise ValueError('Probe log has no ACT_ONE_ROUTE result')
    logged_seed, tiles, waypoint, ticks, jumps, reached = map(int, match.groups())
    # `reached_exit` comes from world_goal_touched(), the physical overlap test
    # against the exit marker. A shortest tile route is only a waypoint guide:
    # a valid jump/drop can touch the exit before every guide tile is visited.
    if logged_seed != seed or reached != 1 or not tiles or ticks < 1 or waypoint > tiles:
        raise ValueError('Route probe did not reach the requested floor exit')
    player_match = re.search(
        r'(?m)^ACT_ONE_PLAYER class=(\d+) hp=(\d+)/(\d+) bow=(\d+) arrows=(\d+)$', log)
    player_class = int(player_match.group(1)) if player_match else None
    if player_class is not None and player_class not in (0, 1, 2):
        raise ValueError('Route probe reported an unsupported player class')
    boss_report = None
    if require_boss_defeat:
        if enemies is None:
            raise ValueError('Boss route proof requires the exact VWE3 enemy packet')
        boss_match = re.search(
            r'ACT_ONE_BOSS result defeated=(\d+) hp=([0-9.]+) player_hp=(-?\d+) '
            r'ticks=(\d+) waypoint=(\d+)/(\d+)', log)
        if not boss_match:
            raise ValueError('Probe log has no supported boss defeat result')
        defeated, hp, player_hp, boss_ticks, boss_waypoint, boss_route = boss_match.groups()
        if int(defeated) != 1 or float(hp) != 0.0 or int(player_hp) <= 0:
            raise ValueError('Route probe did not defeat the boss while the player survived')
        boss_report = dict(defeated=True, hp=float(hp), player_hp=int(player_hp),
                           ticks=int(boss_ticks), final_waypoint=int(boss_waypoint),
                           route_tiles=int(boss_route))
    if source_root is None:
        source_root = Path(__file__).resolve().parents[1]
    simulator = source_fingerprint(source_root)
    if simulator_files is not None or simulator_sha256 is not None:
        if simulator_files is None or simulator_sha256 is None:
            raise ValueError('Simulator source list and SHA-256 must be provided together')
        selected = sorted(set(simulator_files))
        if len(selected) != len(simulator_files):
            raise ValueError('Simulator source file list contains duplicates')
        source_root = source_root.resolve()
        digest = hashlib.sha256()
        for relative in selected:
            path = (source_root / relative).resolve()
            if not path.is_relative_to(source_root) or not path.is_file():
                raise ValueError(f'Invalid simulator source path: {relative}')
            digest.update(relative.encode('utf-8'))
            digest.update(b'\0')
            digest.update(path.read_bytes())
            digest.update(b'\0')
        if digest.hexdigest() != simulator_sha256:
            raise ValueError('Simulator sources changed after the probe was compiled')
        simulator = dict(sha256=simulator_sha256, files=selected)
    trace_masks = read_trace(trace_path) if trace_path else None
    if trace_masks is not None and len(trace_masks) != ticks:
        raise ValueError('Controller trace frame count does not match the successful route log')
    report = dict(format='act-one-native-route-proof',
                  version=5 if player_class is not None else (4 if enemies is not None else (3 if trace_path else 2)), seed=seed,
                  reached_exit=True, route_tiles=tiles, final_waypoint=waypoint,
                  ticks=ticks, jumps=jumps,
                  world_sha256=hashlib.sha256(world.read_bytes()).hexdigest(),
                  entities_sha256=hashlib.sha256(entities.read_bytes()).hexdigest(),
                  simulator=simulator)
    if player_class is not None:
        report['player_class'] = player_class
        report['start_state'] = dict(hp=int(player_match.group(2)),
                                     max_hp=int(player_match.group(3)),
                                     arrows=int(player_match.group(5)))
    if enemies is not None:
        report['enemies_sha256'] = hashlib.sha256(enemies.read_bytes()).hexdigest()
    if boss_report is not None:
        report['boss'] = boss_report
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
    parser.add_argument('--enemies', type=Path,
                        help='exact VWE3 enemy packet used for a boss route proof')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seed', type=int, required=True)
    parser.add_argument('--trace', type=Path,
                        help='successful VTR1 input trace produced with --trace-out by the C route probe')
    parser.add_argument('--require-boss-defeat', action='store_true',
                        help='require the route log to prove the player defeated the boss and survived')
    parser.add_argument('--source-root', type=Path,
                        help='repository root containing the recorded simulator sources')
    parser.add_argument('--simulator-files', nargs='+',
                        help='exact relative source list used for simulator fingerprinting')
    parser.add_argument('--simulator-sha256',
                        help='source digest captured when the probe was compiled')
    args = parser.parse_args()
    print(json.dumps(record(args.log, args.world, args.entities,
                            args.output, args.seed, trace_path=args.trace,
                            require_boss_defeat=args.require_boss_defeat,
                            source_root=args.source_root,
                            simulator_files=args.simulator_files,
                            simulator_sha256=args.simulator_sha256,
                            enemies=args.enemies), indent=2))


if __name__ == '__main__':
    main()
