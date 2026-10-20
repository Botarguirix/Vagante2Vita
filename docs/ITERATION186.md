# Iteration 01.86 — cave entry and floor-zero verification

VPK: `_local/build-act-one-186/Vagante2Vita-iteration186.vpk`  
SHA-256: `1C8B417AACA57B90E65D66C3FB71D3BDD431E21B75D077575933BDB99FF5B749`

## Vita3K verification

Tests used the Vita3K files from `C:\Vita3k` through the isolated copy and profile at `G:\VaganteVitaVita3K-iter167`. The source installation was not used as the test profile.

The direct ACT_ONE floor-zero route passed in Vita3K. It defeated the Dragon, reached the exit, and loaded floor one. The cave-entry route also passed: the intro reached the cave, entered ACT_ONE, defeated the Dragon, reached the exit, and loaded floor one. Its replay now removes the redundant final RIGHT from the intro trace and reproduces the six CROSS/jump ticks used by direct floor entry.

The three-floor campaign remains unverified in Vita3K. Fresh Rogue traces for all three floors defeated their bosses in the native host simulator, but the long Vita3K campaign replay did not produce a completion or transition marker before its log stopped advancing. The campaign result therefore remains failed; this iteration does not claim that floor one, floor two, or campaign completion work in the emulator. Boss behavior and floor generation still include provisional reconstructions, and physical Vita hardware has not been tested.

## Checks

- Vita3K direct ACT_ONE floor zero: passed, `G:\VaganteVitaVita3K-iter167\runs\iteration186\run-000003\result.json`.
- Vita3K intro cave entry through floor-zero exit: passed, `G:\VaganteVitaVita3K-iter167\runs\iteration186\run-000004\result.json`.
- Three-floor Rogue route proof: all three floors defeated their bosses in the host simulator, `_local/routes-act-one-186-rogue/route-results.json`.
- Vita3K three-floor campaign replay: incomplete and failed, `G:\VaganteVitaVita3K-iter167\runs\iteration186\run-000002\result.json`.
- Python suite: 218 passed.
- Windows host suite: 13 passed.
