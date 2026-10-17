"""Make an explicit one-shot controller sequence; no game resources included."""
import argparse
import json
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


def append_trace(steps, buttons_by_tick):
    """Run-length encode the exact controller input recorded by player_step."""
    for buttons in buttons_by_tick:
        if steps and steps[-1][0] == buttons and steps[-1][1] < 600:
            prior_buttons, ticks = steps[-1]
            steps[-1] = (prior_buttons, ticks + 1)
        else:
            steps.append((buttons, 1))


def campaign_route_traces(campaign: Path):
    """Use traces only when a proof matches its exact floor and simulator."""
    if (campaign / 'manifest.json').is_file():
        validate_manifest(campaign)
    traces = {}
    floors = {
        0: (campaign / 'act-one-world.vwi', campaign / 'act-one-entities.vwe'),
        1: (campaign / 'floors/floor-01.vwi', campaign / 'floors/floor-01-entities.vwe'),
        2: (campaign / 'floors/floor-02.vwi', campaign / 'floors/floor-02-entities.vwe'),
    }
    for index, (world_path, entities_path) in floors.items():
        proof_path = campaign / f'route-proofs/floor-{index:02d}.json'
        if not proof_path.is_file():
            continue
        world, entities = world_path.read_bytes(), entities_path.read_bytes()
        proof = load_route_proof(proof_path, world, entities)
        if proof['version'] == 3:
            traces[index] = read_trace(proof_path.with_suffix('.vtr'))
    return traces


def write_replay(output: Path, steps):
    if not steps or len(steps) > 1024:
        raise ValueError('replay must contain 1..1024 VTS1 steps')
    if any(ticks < 1 or ticks > 600 for _, ticks in steps) or sum(t for _, t in steps) > 36000:
        raise ValueError('replay exceeds VTS1 tick limits')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(struct.pack('<4sHH', b'VTS1', len(steps), 0)
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
    parser.add_argument('--layout', type=Path,
                        help='One ACT_ONE floor layout JSON containing native_tile_path.path')
    parser.add_argument('--campaign', type=Path,
                        help='Private campaign package root with three floor layouts')
    args = parser.parse_args()

    if args.act_one_exit and args.act_one_campaign_exit:
        parser.error('choose only one ACT_ONE replay mode')
    if args.act_one_exit or args.act_one_campaign_exit:
        try:
            if args.act_one_campaign_exit:
                if not args.campaign:
                    parser.error('--campaign is required with --act-one-campaign-exit')
                campaign = args.campaign.resolve()
                documents = campaign_layouts(campaign)
                traces = campaign_route_traces(campaign)
            else:
                if not args.layout:
                    parser.error('--layout is required with --act-one-exit')
                documents = [json.loads(args.layout.read_text(encoding='utf-8'))]
            steps = []

            def tap(buttons, rest=12):
                steps.extend(((buttons, 6), (0, rest)))

            # Start new game and choose Knight once, then follow each floor path.
            tap(CROSS, 110)
            tap(CROSS, 30)
            routes = []
            route_sources = []
            for index, document in enumerate(documents):
                route = route_from_layout(document)
                routes.append(route)
                if args.act_one_campaign_exit and index in traces:
                    append_trace(steps, traces[index])
                    route_sources.append(f'player_step:{index}')
                else:
                    append_route(steps, route)
                    route_sources.append(f'tile-estimate:{index}')
                if args.act_one_campaign_exit:
                    # Let the runtime finish loading the next floor before
                    # sending the next directional input segment.
                    steps.append((0, 60))
            write_replay(args.output, steps)
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
    write_replay(args.output, steps)
    print(f'REPLAY steps={len(steps)} nominal_ticks={sum(t for _, t in steps)} assertions_on_host=1')


if __name__ == '__main__':
    main()
