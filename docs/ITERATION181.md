# Iteration 01.81 — native first-floor transition

VPK: `_local/build-act-one-181/Vagante2Vita-iteration181.vpk`  
SHA-256: `20A8B269DD8679A3D2B24D9B4E10D8CCDECF1BC109B4AC163171C4585E1A1300`

## Vita3K verification

Vita3K was sourced from `C:\Vita3k`; the test ran in the isolated clone and copied profile at `G:\VaganteVitaVita3K-iter167`. The original source instance was left untouched. The test played a recorded Knight route through ACT_ONE floor 0, required the Dragon defeat, and verified the exit gate by loading floor 1. The runtime log records the boss defeat, the floor transition, and 25 HP on arrival at floor 1. All native assertions passed.

The replay format now supports a stop-after-first-floor flag, and the route preparer can use a hash-bound physical controller trace for one floor. This makes the floor-zero test independently verifiable instead of letting failures on later floors hide whether the first floor works. The checker also recognizes the runtime's full `WORLD_ENEMIES` log line and requires a recorded Dragon defeat before accepting this transition test.

ACT_ONE as a complete three-floor campaign remains incomplete. Previous full-route attempts diverged or failed in host simulation on later floors; no Vita3K evidence yet proves floor 1→2, all three boss gates, or campaign completion. Enemy and boss behavior still includes provisional models, and physical Vita hardware has not been tested.

## Checks

- Vita3K first-floor transition: passed, route source `player_step:0` (`G:\VaganteVitaVita3K-iter167\runs\iteration181\run-000001\result.json`).
- Windows host tests: 13 passed (`_local/host-tests-iteration181/report.json`).
- Replay and Vita3K checker tests: 24 passed.
