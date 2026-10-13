"""ACT_ONE secret/pedestal graph stages; tiles and playable floor still pending."""
import argparse
import json
from pathlib import Path

try:
    from tools.floor_main_path import MT19937
    from tools.floor_treasure import treasure_graph
    from tools.generation_random import rand_float, shuffle
except ModuleNotFoundError:
    from floor_main_path import MT19937
    from floor_treasure import treasure_graph
    from generation_random import rand_float, shuffle

SHOP, TREASURE, BRIGHT, BOOK = 0x100, 0x200, 0x400, 0x4000
SPECIAL = SHOP | TREASURE | BOOK


def secret_neighbors(types, x, y, act_index=0):
    if types[y][x] & 15:
        return []
    neighbors = []
    for nx, toward_source in ((x - 1, 4), (x + 1, 1)):
        if not 0 <= nx < len(types[y]):
            continue
        kind = types[y][nx]
        if (kind & 15 and not kind & (toward_source | SPECIAL)
                and not (act_index == 1 and kind & BRIGHT)):
            neighbors.append((nx, y))
    return neighbors


def place_secrets(types, requested, rng, act_index=0):
    if type(requested) is not int or requested < 0:
        raise ValueError('Secret count must be a nonnegative integer')
    result = dict(requested=requested, candidates_initial=0, placements=[])
    if not requested:
        return result
    candidates = [(x, y) for x in range(len(types[0])) for y in range(len(types))
                  if secret_neighbors(types, x, y, act_index)]
    result['candidates_initial'] = len(candidates)
    shuffle(rng, candidates)
    for x, y in candidates[:requested]:
        neighbors = secret_neighbors(types, x, y, act_index)
        if not neighbors:
            raise RuntimeError('Cached secret candidate lost all neighbors')
        shuffle(rng, neighbors)
        nx, ny = neighbors[0]
        bit, opposite = (4, 1) if nx > x else (1, 4)
        types[y][x] |= bit
        types[ny][nx] |= opposite
        # isSecret is separate metadata; it is not a bit in room_type.
        result['placements'].append(dict(x=x, y=y, neighbor=[nx, ny],
                                         room_type=types[y][x], is_secret=True))
    return result


def secret_stage(types, rng):
    requested = 1 + rng.game_int() % 2
    roll = rand_float(rng)
    result = place_secrets(types, 0 if roll < 0.33 else requested, rng)
    result.update(requested_before_chance=requested, chance_roll=roll,
                  zero_secret_threshold=0.33)
    return result


def book_room_bases(act):
    return {int(key) & ~BOOK for key in act['rooms'] if int(key) & BOOK}


def place_pedestals(types, allowed, start, goal, rng, requested=1):
    if type(requested) is not int or requested < 0:
        raise ValueError('Pedestal count must be a nonnegative integer')
    result = dict(requested=requested, catalog_bases=sorted(allowed),
                  cells_examined=0, placements=[])
    if not allowed:
        return result
    candidates = [(x, y) for x in range(len(types[0])) for y in range(len(types))]
    shuffle(rng, candidates)
    for x, y in candidates:
        if len(result['placements']) >= requested:
            break
        result['cells_examined'] += 1
        if (x, y) in (tuple(start), tuple(goal)):
            continue
        kind = types[y][x]
        neighbor = None
        if kind & 15:
            if kind not in allowed or kind & SPECIAL:
                continue
            mode = 'existing_room'
        else:
            # Preserve the executable's literal catalog tests: base 2 for the
            # right-neighbor branch, base 0 for the left-neighbor branch.
            # They differ from the horizontal types produced (4 and 1).
            if (2 in allowed and x + 1 < len(types[y])
                    and types[y][x + 1] & 15 and not types[y][x + 1] & SPECIAL):
                types[y][x] |= 4
                types[y][x + 1] |= 1
                neighbor = [x + 1, y]
            elif (0 in allowed and x > 0 and types[y][x - 1] & 15
                  and not types[y][x - 1] & SPECIAL):
                types[y][x] |= 1
                types[y][x - 1] |= 4
                neighbor = [x - 1, y]
            else:
                continue
            mode = 'new_horizontal_room'
        types[y][x] |= BOOK
        result['placements'].append(dict(x=x, y=y, room_type=types[y][x],
                                         mode=mode, neighbor=neighbor))
    return result


def special_graph(seed, floor_index, act):
    report = treasure_graph(seed, floor_index)
    rng = MT19937(seed)
    for _ in range(report['rng_draws']):
        rng.raw()
    if rng.fingerprint() != report['rng_state_sha256']:
        raise RuntimeError('Treasure-stage RNG resume mismatch')
    types = report['room_types_row_major']
    report['pre_secret_rng_draws'] = rng.draws
    report['secrets'] = secret_stage(types, rng)
    report['pre_pedestal_rng_draws'] = rng.draws
    report['pedestals'] = place_pedestals(types, book_room_bases(act), report['start'],
                                         report['goal'], rng)
    report['links_row_major'] = [[v & 15 for v in row] for row in types]
    report['rng_draws'] = rng.draws
    report['rng_state_sha256'] = rng.fingerprint()
    report['secret_stage_implemented'] = report['pedestal_stage_implemented'] = True
    report['stage'] = 'special_room_graph_only'
    report['floor_complete'] = False
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rooms', type=Path, help='Private original rooms.json')
    parser.add_argument('--seed', required=True, type=int)
    parser.add_argument('--floor', default=0, type=int)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    database = json.loads(args.rooms.read_text('utf-8'))
    report = special_graph(args.seed, args.floor, database['acts'][0])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f"SPECIAL_GRAPH seed={args.seed} floor={args.floor} "
          f"secrets={len(report['secrets']['placements'])} "
          f"pedestals={len(report['pedestals']['placements'])} floor_complete=0")


if __name__ == '__main__':
    main()
