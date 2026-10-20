# Iteration 01.76 — Vita3K smoke and campaign diagnostics

VPK: `_local/build-act-one-176/Vagante2Vita-iteration176.vpk`  
SHA-256: `59F7D73E1025A4D50A8E315A125C6A4540D01EC44B8917B12C7F010EA63B24DF`

## Vita3K verification

The VPK was tested with `C:\Vita3k` as the source and the isolated profile/runtime at `G:\VaganteVitaVita3K-iter167`. The source instance was not modified.

The basic native smoke passed: expected title/version, archive/world load, all three classes and base HP, movement, jump, character panel, pause/resume, Rogue bow, replay consumption, and normal world exit. The test runner stopped the isolated frontend after completion.

The ACT_ONE three-floor campaign smoke remains unverified. Iterations 01.69 through 01.75 loaded the expected title and world but timed out before the replay completion marker and floor-one transition. OpenGL/Vulkan and Qt/SDL comparisons did not yield a passing transition. The route trace alignment fix is retained; it enabled native Dragon damage, but it is not evidence of a completed boss fight. Details are in [ITERATION169.md](ITERATION169.md).

## Other checks

- Windows host tests: 13 passed (`_local/host-tests-iteration176/report.json`).
- `tests/test_prepare_smoke_replay.py`: 8 passed.
- Hardware Vita behavior/performance has not been verified.

01.76 removes an unproven frame-scheduler recovery change from the diagnostic builds. Campaign replay logging is sampled to retain periodic boss/progress/transition events while suppressing per-frame movement output on long traces.
