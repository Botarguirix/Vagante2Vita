# Iteration 01.80 — isolated Vita3K smoke

VPK: `_local/build-act-one-180/Vagante2Vita-iteration180.vpk`  
SHA-256: `A4BE0FCD7A48322768DA3A9620F625F58D127A608848BDE5873442C47CB64220`

## Vita3K verification

The test used `C:\Vita3k` as the source and ran the copied executable, firmware, profile, and game data from `G:\VaganteVitaVita3K-iter167`. The source instance was not modified. Vita3K 0.2.1.4194 used the Qt frontend and OpenGL renderer.

The native smoke passed. It loaded the expected 01.80 title and ACT_ONE data, consumed the replay, and passed checks for all three classes, base HP, movement, jump, character panel pages, pause/resume, Rogue arrows, attack geometry, and normal world exit. The harness stopped the isolated process after the replay.

The test exposed a replay deadlock in the character panel: the frame accumulator was reset on every loop while the panel remained open. It now resets only when the panel opens or closes, allowing replay time to continue. The same VBlank issue found during 01.79 campaign playback was avoided by yielding during automated replay; normal play still uses the display VBlank wait.

ACT_ONE campaign completion remains unverified. The 01.79 campaign replay loaded the correct title and floor data, then the Rogue died to an enemy on floor 0 at player tick 1192 before the first floor transition. The corresponding host route proof had predicted a boss defeat and exit with 10 HP, so that proof does not establish Vita3K parity. Separate Knight and Mage route probes also failed to finish all three floors.

## Checks

- Windows host tests: 13 passed (`_local/host-tests-iteration180/report.json`).
- `tests/test_prepare_smoke_replay.py`: 8 passed.
- Vita3K smoke result: `G:\VaganteVitaVita3K-iter167\runs\iteration180\run-000001\result.json`.
- Hardware Vita behavior and performance have not been verified.
