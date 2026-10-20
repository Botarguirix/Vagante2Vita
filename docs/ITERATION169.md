# Iteration 01.69 — Vita3K campaign replay alignment

VPK: `Vagante2Vita-iteration169.vpk`  
SHA-256: `7163084692C8D9EF702A3ECEBE390D9908294AC2EF492071C0ED74D49DDF2463`

## Test setup

The test harness used `C:\Vita3k` as its Vita3K source and ran the verified, isolated copy at `G:\VaganteVitaVita3K-iter167`. The shared source instance was not modified. The campaign package was the private three-floor bundle `campaign-act-one-wormstats-166`; the replay selected Rogue and used hash-bound `player_step` traces on all floors.

## Change and result

`prepare_smoke_replay.py` no longer inserts 30 neutral world ticks after class confirmation and before the recorded floor trace. That gap shifted the recorded inputs away from the physics state they were captured against. The replay now starts the trace immediately after the six-tick class-confirmation input.

The rerun loaded the expected title and ACT_ONE world. Native telemetry confirmed that the Rogue damaged the Dragon: its health fell from 65 to 1.8 by world tick 1167. The player had 15 HP. Vita3K then stopped advancing the native log at tick 1185 while its isolated process remained alive; the 9,487-tick replay was not consumed, and no floor transition was recorded. The isolated test process was stopped after confirming it remained stalled. The campaign smoke therefore **fails**; this run does not establish a completed first floor.

## Verification

- `tests/test_prepare_smoke_replay.py`: 8 passed.
- Windows host tests: 13 passed (`_local/host-tests-iteration169-replayalign/report.json`).
- Vita3K campaign smoke: failed at Dragon encounter; expected title/version and ACT_ONE load passed, but replay completion and floor transitions did not.

Next investigation should focus on why the native simulation stops advancing during the Dragon encounter, then rerun the same isolated Vita3K campaign trace to verify the first-floor transition.

## Follow-up diagnostics (01.70–01.75)

The same replay was compared across OpenGL, Vulkan, Qt, and SDL on the isolated copy. Campaign assertions remained false and the process timed out; the expected title and world still loaded. The 01.70/01.71 frame markers showed `draw_world`, framebuffer submission, and several vblank waits returning, while the catch-up scheduler needed only one or two frame increments. The speculative scheduler rebase was removed in 01.76.

The replay trace alignment remains correct and the Dragon takes damage. The detailed per-hit and player telemetry was reduced for campaign replays to limit log traffic, but 01.72–01.75 still timed out without proving a floor transition. Some collected logs end mid-line because the harness stops the emulator on timeout before its stdio buffer can close; those partial lines do not establish where execution stopped. SDL runs without concurrent log polling also timed out, so the Qt log watcher alone does not explain the failure. No campaign-completion claim is supported by these runs.

01.75 verification: 13 Windows host tests passed. The Vita3K campaign result remains failed (`campaign_replay_consumed=false`, `floor_one_transition=false`).
