#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if (( $# < 2 || $# > 5 )); then
  echo "usage: $0 WORLD.VWI ENTITIES.VWE [SEED] [OUTPUT_DIRECTORY] [TRACE.VTR]" >&2
  exit 2
fi

world=$(realpath -- "$1")
entities=$(realpath -- "$2")
seed=${3:-12345}
output_directory=${4:-"$repo_root/_local/host-route"}
trace_output=${5:-}
mkdir -p -- "$output_directory"

cc -std=c11 -O2 -Wall -Wextra -Werror -I "$repo_root/vita/src" \
  "$repo_root/vita/src/act_one_route_probe.c" \
  "$repo_root/vita/src/world.c" \
  "$repo_root/vita/src/world_entities.c" \
  "$repo_root/vita/src/world_combat.c" \
  "$repo_root/vita/src/loot.c" \
  "$repo_root/vita/src/player.c" \
  "$repo_root/vita/src/player_stats.c" \
  "$repo_root/vita/src/melee.c" \
  "$repo_root/vita/src/bow.c" \
  "$repo_root/vita/src/input_replay.c" \
  -lm -o "$output_directory/act-one-route-probe"

if [[ -n "$trace_output" ]]; then
  mkdir -p -- "$(dirname -- "$trace_output")"
  if [[ -e "$trace_output" ]]; then
    echo "trace output already exists: $trace_output" >&2
    exit 2
  fi
  "$output_directory/act-one-route-probe" "$world" "$entities" "$seed" \
    --trace-out "$trace_output" | tee "$output_directory/act-one-route.log"
else
  "$output_directory/act-one-route-probe" "$world" "$entities" "$seed" \
    | tee "$output_directory/act-one-route.log"
fi
