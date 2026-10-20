# Iteration 01.96 — current ACT_ONE validation

Build used: `_local/build-act-one-196-diag/Vagante2Vita-iteration196.vpk`  
SHA-256: `EB412329A7E076C6CD7DD129B2A43B1825B7991A12BA2B1C2E721C2015851B41`

This is a diagnostic build, not a release candidate. The runtime currently has
tick-1178 diagnostics enabled in `world_combat.c` to locate the intermittent
Rogue replay stall.

## Verified

- Windows C host suite: 13/13 passed, including campaign flow, world entities,
  melee, player, and world extent (`_local/host-tests-windows/report.json`).
- Python suite: 220 passed, with 70 subtests passed.
- Three-floor Rogue route proofs passed in the host route simulator. Seeds were
  1, 49, and 4 for floors 0, 1, and 2 (`_local/routes-act-one-194-bossclass1/route-results.json`).
- The exact three-floor delivery package passed the ACT_ONE runtime asset audit
  (`python tools/audit_act_one_runtime_assets.py _local/campaign-act-one-194-rogue-current`).
- Vita3K 0.2.1.4194, isolated from the source installation at `C:\Vita3k`:
  the Knight floor-zero replay passed, defeated the boss, reached the exit, and
  loaded floor one (`_local/vita3k-c-drive-light-194/runs/iteration196/run-000002/result.json`).
  Its diagnostic trace completed every observed stage at tick 1,178.

## Not verified or incomplete

- The Rogue floor-zero replay did not create a native session log within 300
  seconds (`run-000003`). A visible rerun left a Windows “Vita3K.exe - Error de
  la aplicación” dialog before a session log was created (`run-000004`). The
  dialog could not be captured reliably, so its exception details are unknown.
- The full three-floor campaign has not passed in Vita3K. Host route proofs do
  not establish native campaign completion.
- Intro-to-cave entry for Rogue, physical Vita hardware performance, and
  controller behavior have not been verified in this iteration.
- Floor generation and enemy/boss behavior remain partial reconstructions;
  runtime parity with the original game is not claimed.

Next step: inspect the Windows Vita3K error dialog after it is closed, stabilize
the Rogue launch/replay, then run the full three-floor campaign in the isolated
profile. Remove the temporary tick diagnostic before producing a release VPK.
