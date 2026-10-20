"""Make an explicit one-shot controller sequence; no game resources included."""
import argparse
import json
import re
import struct
from pathlib import Path

try:
    from tools.prepare_act_one_campaign import load_route_proof
    from tools.prepare_world_bundle import validate_manifest
    from tools.route_trace import read_trace
except ModuleNotFoundError:
    from prepare_act_one_campaign import load_route_proof
    from prepare_world_bundle import validate_manifest
    from route_trace import read_trace

CROSS = 0x4000
REPLAY_LIMIT = 2048
SQUARE = 0x8000
CIRCLE = 0x2000
TRIANGLE = 0x1000
UP = 0x10
RIGHT = 0x20
DOWN = 0x40
LEFT = 0x80
R = 0x200
SELECT = 1
START = 8


def route_from_layout(document):
    if 'tile_layout' in document:
        layout = document['tile_layout']
        route = layout.get('native_tile_path', {}).get('path')
        entrance = layout.get('endpoints', {}).get('entrance')
        goal = layout.get('endpoints', {}).get('goal')
    else:
        layout = document
        route = layout.get('native_tile_path', {}).get('route_tiles')
        entrance = layout.get('entrance')
        goal = layout.get('goal')
    if not route or route[0] != entrance or route[-1] != goal:
        raise ValueError('layout has no route matching its entrance and goal')
    for (x, y), (nx, ny) in zip(route, route[1:]):
        if abs(nx - x) + abs(ny - y) != 1:
            raise ValueError('route contains non-adjacent tiles')
    return route


def append_route(steps, route):
    # Estimated directional input from tile geometry. Runtime contact and
    # transition logs remain the success criteria because this ignores physics.
    index = 0
    while index < len(route) - 1:
        x, y = route[index]
        nx, ny = route[index + 1]
        dx, dy = nx - x, ny - y
        count = 1
        while index + count < len(route) - 1:
            ax, ay = route[index + count - 1]
            bx, by = route[index + count]
            cx, cy = route[index + count + 1]
            if (bx - ax, by - ay) != (cx - bx, cy - by):
                break
            count += 1
        if dx:
            steps.append((RIGHT if dx > 0 else LEFT, max(8, count * 10)))
        elif dy < 0:
            steps.append((UP, max(8, count * 10)))
        else:
            steps.extend(((DOWN | CROSS, 4), (0, 10)))
        index += count


def append_trace(steps, buttons_by_tick, merge_first=True):
    """Run-length encode the exact controller input recorded by player_step."""
    for index, buttons in enumerate(buttons_by_tick):
        if (steps and (index > 0 or merge_first)
                and steps[-1][0] == buttons and steps[-1][1] < 600):
            prior_buttons, ticks = steps[-1]
            steps[-1] = (prior_buttons, ticks + 1)
        else:
            steps.append((buttons, 1))


def campaign_route_traces(campaign: Path, player_class: int | None = None):
    """Use traces only when a proof matches its exact floor and simulator."""
    if (campaign / 'manifest.json').is_file():
        validate_manifest(campaign)
    traces = {}
    floors = {
        0: (campaign / 'act-one-world.vwi', campaign / 'act-one-entities.vwe',
            campaign / 'act-one-enemies.vwe'),
        1: (campaign / 'floors/floor-01.vwi', campaign / 'floors/floor-01-entities.vwe',
            campaign / 'floors/floor-01-enemies.vwe'),
        2: (campaign / 'floors/floor-02.vwi', campaign / 'floors/floor-02-entities.vwe',
            campaign / 'floors/floor-02-enemies.vwe'),
    }
    for index, (world_path, entities_path, enemies_path) in floors.items():
        proof_path = campaign / f'route-proofs/floor-{index:02d}.json'
        if not proof_path.is_file():
            continue
        world, entities = world_path.read_bytes(), entities_path.read_bytes()
        enemies = enemies_path.read_bytes()
        proof = load_route_proof(proof_path, world, entities, enemies=enemies)
        if proof['version'] == 5 and player_class is not None and proof['player_class'] != player_class:
            raise ValueError(f'{proof_path}: route trace was recorded for class {proof["player_class"]}, not {player_class}')
        if proof['version'] in (3, 4, 5):
            traces[index] = read_trace(proof_path.with_suffix('.vtr'))
    return traces


def route_trace_start_tick(path: Path) -> int:
    """Return the player tick at which a recorded route probe began."""
    metadata_path = path.with_suffix('.json')
    if not metadata_path.is_file():
        return 0
    metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
    text = json.dumps(metadata)
    match = re.search(r'player_step(?:_start)?_tick[\"=: ]+(\d+)', text)
    if match:
        return int(match.group(1))
    match = re.search(r'player_ticks[\"=: ]+(\d+)', text)
    return int(match.group(1)) if match else 0


def write_replay(output: Path, steps, start_intro=False, stop_after_first_floor=False,
                 fast_forward=False):
    if not steps or len(steps) > REPLAY_LIMIT:
        raise ValueError(f'replay must contain 1..{REPLAY_LIMIT} VTS1 steps')
    if any(ticks < 1 or ticks > 600 for _, ticks in steps) or sum(t for _, t in steps) > 36000:
        raise ValueError('replay exceeds VTS1 tick limits')
    output.parent.mkdir(parents=True, exist_ok=True)
    flags = int(start_intro) | (int(stop_after_first_floor) << 1) | (int(fast_forward) << 2)
    output.write_bytes(struct.pack('<4sHH', b'VTS1', len(steps), flags)
                       + b''.join(struct.pack('<IHH', buttons, ticks, 0)
                                  for buttons, ticks in steps))


def campaign_layouts(campaign: Path):
    paths = [campaign / 'act-one-layout.json']
    paths.extend(campaign / 'floors' / f'floor-{index:02d}-layout.json'
                 for index in (1, 2))
    missing = [path for path in paths if not path.is_file()]
    if missing:
        raise FileNotFoundError(f'campaign package is missing floor layouts: {missing}')
    return [json.loads(path.read_text(encoding='utf-8')) for path in paths]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--act-one-exit', action='store_true',
                        help='Navigate the selected Knight through one ACT_ONE floor exit')
    parser.add_argument('--act-one-campaign-exit', action='store_true',
                        help='Attempt exits on ACT_ONE floors 0, 1, and 2')
    parser.add_argument('--intro-cave-entry', action='store_true',
                        help='Traverse the intro cave, enter ACT_ONE and clear its first floor')
    parser.add_argument('--intro-trace', type=Path,
                        help='VTR1 controller trace from the native intro route probe')
    parser.add_argument('--route-trace', type=Path,
                        help='VTR1 controller trace for a single ACT_ONE floor')
    parser.add_argument('--layout', type=Path,
                        help='One ACT_ONE floor layout JSON containing native_tile_path.path')
    parser.add_argument('--campaign', type=Path,
                        help='Private campaign package root with three floor layouts')
    parser.add_argument('--player-class', type=int, choices=(0, 1, 2), default=0,
                        help='Select Knight (0), Rogue (1), or Mage (2) for campaign route traces')
    parser.add_argument('--fast-forward', action='store_true',
                        help='Run one deterministic simulation tick per loop without rendering')
    args = parser.parse_args()

    if sum((args.act_one_exit, args.act_one_campaign_exit, args.intro_cave_entry)) > 1:
        parser.error('choose only one world traversal replay mode')
    if args.intro_cave_entry:
        try:
            if not args.campaign or not args.intro_trace:
                parser.error('--campaign and --intro-trace are required with --intro-cave-entry')
            intro_masks = read_trace(args.intro_trace)
            # The native intro route touches the cave one tick before its
            # 38-frame trace ends. Drop that redundant final RIGHT so the
            # following action starts cleanly on ACT_ONE floor 0.
            if intro_masks[-1] == RIGHT:
                intro_masks = intro_masks[:-1]
            floor_zero_masks = (read_trace(args.route_trace) if args.route_trace
                                else campaign_route_traces(args.campaign.resolve(), args.player_class).get(0))
            if floor_zero_masks is None:
                parser.error('campaign package has no valid floor-0 player_step trace')
            steps = []
            steps.extend(((CROSS, 6), (0, 110)))
            for _ in range(args.player_class):
                steps.extend(((RIGHT, 6), (0, 12)))
            steps.extend(((CROSS, 6), (0, 30)))
            append_trace(steps, intro_masks)
            # Match direct floor-entry playback, where the six class-accept
            # ticks become a jump at the beginning of the world simulation.
            steps.append((CROSS, 6))
            append_trace(steps, floor_zero_masks, merge_first=False)
            write_replay(args.output, steps, start_intro=True, fast_forward=args.fast_forward)
        except (OSError, ValueError, KeyError) as error:
            parser.error(str(error))
        print(f'REPLAY INTRO_CAVE_ENTRY steps={len(steps)} nominal_ticks={sum(t for _, t in steps)} '
              f'intro_ticks={len(intro_masks)} act_one_floor0_ticks={len(floor_zero_masks)} '
              'act_one_entry_jump_ticks=6 '
              'route_sources=intro:player_step,act-one:player_step start_intro=1')
        return
    if args.act_one_exit or args.act_one_campaign_exit:
        try:
            if args.act_one_campaign_exit:
                if not args.campaign:
                    parser.error('--campaign is required with --act-one-campaign-exit')
                campaign = args.campaign.resolve()
                documents = campaign_layouts(campaign)
                traces = campaign_route_traces(campaign, args.player_class)
            else:
                if not args.layout:
                    parser.error('--layout is required with --act-one-exit')
                documents = [json.loads(args.layout.read_text(encoding='utf-8'))]
            steps = []

            def tap(buttons, rest=12):
                steps.extend(((buttons, 6), (0, rest)))

            def accept_class():
                # The accepted input changes the scene during this six-tick
                # hold. Keep the recorded player_step trace adjacent so its
                # first mask is applied on the first world simulation tick.
                steps.append((CROSS, 6))

            # Start new game and choose Knight once, then follow each floor path.
            tap(CROSS, 110)
            for _ in range(args.player_class):
                tap(RIGHT)
            accept_class()
            routes = []
            route_sources = []
            for index, document in enumerate(documents):
                route = route_from_layout(document)
                routes.append(route)
                if args.act_one_exit and args.route_trace:
                    append_trace(steps, read_trace(args.route_trace), merge_first=False)
                    route_sources.append(f'player_step:{index}')
                elif args.act_one_campaign_exit and index in traces:
                    append_trace(steps, traces[index], merge_first=False)
                    route_sources.append(f'player_step:{index}')
                else:
                    append_route(steps, route)
                    route_sources.append(f'tile-estimate:{index}')
            write_replay(args.output, steps, stop_after_first_floor=args.act_one_exit,
                         fast_forward=args.fast_forward)
        except (OSError, ValueError, KeyError) as error:
            parser.error(str(error))
        mode = 'ACT_ONE_CAMPAIGN_EXIT' if args.act_one_campaign_exit else 'ACT_ONE_EXIT'
        print(f'REPLAY {mode} steps={len(steps)} nominal_ticks={sum(t for _, t in steps)} '
              f'route_tiles={sum(len(route) for route in routes)} floors={len(routes)} '
              f'route_sources={",".join(route_sources)}')
        return

    steps = []

    def hold(buttons, ticks):
        steps.append((buttons, ticks))

    def tap(buttons, rest=12):
        hold(buttons, 6)
        hold(0, rest)

    hold(0, 90)
    tap(CROSS, 110)
    tap(CROSS, 30)
    # Knight: traversal, jump, melee, sheet pages, pause and return.
    hold(RIGHT, 60)
    hold(RIGHT | CROSS, 6)
    hold(RIGHT, 30)
    hold(0, 30)
    tap(SQUARE, 80)
    tap(SELECT)
    tap(RIGHT)
    tap(RIGHT)
    tap(CIRCLE)
    tap(START, 45)
    tap(START)
    tap(TRIANGLE, 30)
    # Rogue: base melee and charged projectile.
    tap(RIGHT)
    tap(CROSS, 30)
    hold(R, 90)
    hold(0, 50)
    tap(SQUARE, 50)
    tap(SELECT)
    tap(RIGHT)
    tap(CIRCLE)
    tap(TRIANGLE, 30)
    # Mage: charged Rod sequence, consult sheet and finish normally.
    tap(RIGHT)
    tap(CROSS, 30)
    hold(SQUARE, 90)
    hold(0, 80)
    tap(SELECT)
    tap(CIRCLE)
    hold(0, 90)
    write_replay(args.output, steps, fast_forward=args.fast_forward)
    print(f'REPLAY steps={len(steps)} nominal_ticks={sum(t for _, t in steps)} assertions_on_host=1')


if __name__ == '__main__':
    main()
