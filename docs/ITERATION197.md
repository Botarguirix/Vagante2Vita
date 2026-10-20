# Iteration 01.97 — clean build after diagnostics

VPK: `_local/build-act-one-197-clean/Vagante2Vita-iteration197.vpk`  
SHA-256: `c11cc494d5b6e0137327d1e3437424e89085ed69b0c4fc9eef33c5df5301b03c`  
Size: 847,869 bytes.

This build removes the temporary tick-1,178 tracing from 01.96. It keeps the
current ACT_ONE runtime and replay changes. WSL returned
`Wsl/EnumerateDistros/Service/E_ACCESSDENIED`, so the VPK was built with the
installed Windows VitaSDK toolchain.

## Checks

- VitaSDK Windows build completed for `VAGV00006`, version `01.97`.
- The VPK archive contains `eboot.bin` and `sce_sys/param.sfo` and opens as a
  seven-entry ZIP archive.
- Windows C host suite: 13/13 passed (`_local/host-tests-windows/report.json`).
- Python suite: 222 passed.
- Vita3K 0.2.1.4194 was launched with the 01.97 Knight floor-zero replay in the
  isolated profile, but the process remained idle and created no native session
  log. The attempt failed before native gameplay (`_local/vita3k-c-drive-light-194/runs/iteration197/run-000001/result.json`).
  The unresolved Windows application-error dialog from the prior Rogue attempt
  remained open; its exception details are unknown.

The original 01.94 route proofs were not bound to the VWE3 enemy packet, so they
could incorrectly appear current after a boss placement changed. The proof
format now records the enemy packet SHA-256 and campaign packaging rejects a
boss proof for any other packet. Floor 1's provisional Worm had also been
placed on the entrance tile when native spawn geometry produced no candidate;
it now uses a deterministic late-route fixture position.

The regenerated three-floor campaign passes the host asset audit and Rogue
route simulation with version-4 proofs bound to exact VWI2, VWE2, and VWE3
hashes (`_local/routes-act-one-197-proved-final/route-results.json`). The
smoke-replay generator now supplies each proof's exact VWE3 packet before
accepting its trace; a test confirms that changing the enemy packet invalidates
the route trace.

A full-campaign smoke was staged from this package and launched in an isolated
Vita3K profile cloned from `C:\Vita3k`. Vita3K 0.2.1.4194 stayed alive but idle
for 300 seconds, created no native session log, and left stdout, stderr, and
`vita3k.log` empty (`_local/vita3k-act-one-197-campaign/runs/iteration197/run-000002/result.json`).
This is a pre-native emulator launch failure, not a campaign result. The source
profile is about 2.89 GiB while C: had about 1.39 GiB free, so the smoke used the
existing light-copy mode and did not clone the full user profile.

A debugger run against that isolated executable with `--skip-title` then
reproduced an unhandled access violation before any game was loaded:
`ucrtbase.dll+0x25F31`, return address `Vita3K.exe+0x683965`, with Qt6Core
frames; process exit was `0xC0000005`. The exact reproduction and limits are
recorded in [WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md). This confirms
the current Vita3K blocker is in emulator startup, independent of Vagante's
VPK or campaign resources.

The cave-entry replay is also now assembled from the staged 39-file intro
bundle and the campaign's verified floor-zero V4 trace. It contains 224 steps
(1,463 simulation ticks), forces the intro scene, touches the cave exit, then
plays the floor-zero route and boss encounter. Its SHA-256 is
`20399474c59f5962ffcd8aef004b6f10bbac3fd2e83a37a9805a01135172b14f` at
`_local/vita3k-act-one-197-campaign/intro-entry-combined.vts`. This makes the
user-reported cave-entry path reproducible in the harness, but it is still
unverified in Vita3K until the emulator creates a native session log.

Each host floor was simulated independently; that does not verify persistent
player state or transitions across the complete campaign. The Worm position is
a controlled test fixture, not recovered native spawn parity. Physical Vita
gameplay and Vita3K campaign execution remain unverified. See
[ITERATION196.md](ITERATION196.md) for the last successful per-class emulator
control.

## Integrated route audit after closing the Vita3K error dialog

The 01.97 campaign was retried after the application-error dialog was closed.
The package audit still passed (144 files) and the campaign replay regenerated
at 1,452 steps / 9,299 ticks, but Vita3K 0.2.1.4194 exited with
`0xC0000005` before creating a native session log. The result is recorded in
`_local/vita3k-act-one-197-retry-after-dialog/runs/iteration197/run-000001/`;
this remains an emulator-startup failure, not a game result.

The previously separate route proofs also exposed an integration defect. Their
route runner selected Rogue (class 1) for the Dragon and Worm, but Knight
(class 0) for the Goblin King. The default campaign replay selects Knight. A
direct host playback of the Dragon proof as Knight reached the exit marker
while the Dragon remained alive, so the campaign boss gate would not advance.
For Rogue,
the new integrated host harness loads all three real floor packets, replays the
stored VTR1 inputs, and calls `player_carry_campaign_state` at each transition.
Floor 0 succeeds with 10/80 HP; floor 1 then starts with that carried health
and dies at tick 870 of its 1,660-frame trace, before defeating the Worm. The
standalone floor-1 proof had started at full health and ended at 2 HP. The
campaign route proofs therefore do not establish a playable full campaign.

The reproducible check is `tools/test_act_one_campaign_chain_windows.ps1` and
its C harness is `tests/act_one_campaign_chain_test.c`. The uniform-class route
run is `_local/routes-act-one-197-class1-consistent/route-results.json`; it
passed all three floors independently. Its integrated replay still fails at
floor 1 in `_local/host-campaign-chain-rogue-class1/report.json`. The mixed-class
audit is in `_local/host-campaign-chain-knight/report.json`. These reports show
that the remaining work is broader than changing the transition gate: the
same-class combat routes and carried health/loadout need to be made viable,
then the resulting chain must pass Vita3K after its startup crash is resolved.
No healing at a floor transition is inferred or added without evidence from
the original game.

## Damage breakdown and healing evidence

The source-attributed integrated replay in
`_local/host-chain-contact-source-diagnostics/run.log` shows floor-0 damage is
from the Dragon's body and fireballs, not the common enemies: 70 HP across 13
events, leaving 10/80 HP. Floor 1 carries those 10 HP, the Rogue bow, and 26
arrows; the route then takes Worm contact at ticks 810 and 840, Bat contact at
849, and another Worm hit at 870, dying before the boss. The floor-1 VTR1 has
no range-button presses, so its standalone proof does not demonstrate ranged
combat in the chain. This narrows route work to Dragon avoidance on floor 0 and
combat with carried state on floor 1.

The existing Ghidra export confirms `Player::heal(int)` adds its explicit
argument to current HP, `Player::castHeal(float)` creates a heal spell, and
`Player::doCoinHeal` is tied to coin-heal logic. Those functions do not establish
automatic healing at a floor transition. The saved Ghidra project is owned by
`ikari`; its existing exported pseudocode was used read-only, and the project
was not modified. A temporary route-policy experiment was reverted because
the independent floor-1 route then died at tick 1,327 without defeating its
common enemies. A second experiment allowed bow attacks at nearby enemies and
reached 18 arrow hits, but the Rogue still died at tick 1,724 with the Worm at
26.5 HP. Giving the boss routine priority over nearby enemies kept the player
alive longer, but 28 of 29 arrows hit terrain and the player died at tick
5,191 with the Worm untouched. A kiting variant stalled at the boss waypoint.
All three policy experiments were reverted; the existing route logic remains
unchanged. These results narrow the unresolved work to boss/platform aiming
and surviving contact damage, rather than transition healing alone. The route
planner still needs a combat policy that survives with carried state before
campaign packaging can be called playable.

## Current Worm fixture geometry

The campaign's VWE3 Worm record starts at pixel `(868, 292)` (tile `[54,18]`)
and the provisional movement model settles it near `(864,392)`. In the current
VWI2, the player's route approaches from above and left; the corresponding
solid row at tile y=24 includes x=52–54. The ranged attempt therefore reports
terrain impacts while trying to shoot through that wall, and the route controller
can stall at the waypoint. A class-0 route independently defeats the Dragon
with 25/100 HP, but its Worm route dies at tick 1,591; changing class does not
yet establish a viable campaign chain. These boss coordinates and behavior are
test fixtures, not recovered native spawn or movement parity.

The earlier three-floor host proof in
`_local/routes-worm-campaign-melee-finish-20261018/` uses another campaign
fixture with the Worm at `(436,212)` and passes each floor independently. The
same stored traces were replayed through the integrated harness against that
campaign; Rogue still carries only 10/80 HP from floor 0 and dies to common
enemy contact on floor 1 at tick 334. Changing only the Worm's test placement
therefore does not fix persistent-state viability.

The route policy's melee proximity radius was widened from 20 to 40 pixels for
one test. It made the current floor-1 Rogue die at tick 1,395 rather than
finish at 2 HP, so that change was reverted. The C harness now records the
responsible entity kind and subtype for contact damage, allowing future route
changes to target Dragon fireballs and Worm/Bat contact separately.

A Dragon policy test prevented Rogue from switching to melee within 48 pixels
and added facing input when starting a bow attack. This retained 47 HP at tick
1,200 versus 10 HP on the previous route, but the Dragon still had 17.5 HP
when Rogue died at tick 2,454. A faster 90-tick firing cycle performed worse:
only three arrows hit, the Dragon retained 46.5 HP, and Rogue died at tick
3,093. Both route-policy changes were reverted pending a better ranged/melee
handoff; the isolated route and integrated chain remain unpassed as a campaign.

## Potion-use investigation

The verified rarity table `_local/item-generator-pools-v2.json` identifies
`ITEMTYPE_POTION_HEAL` as item type 6 in `POOL_POTIONS_GOOD` (weight 8).
The verified chest graph routes the root `POOL_CHEST` through weapons, armor,
other items, or wands; the potion pool exists in the item generator but is not
reachable from this chest root. Thus ordinary ACT_ONE chests do not currently
provide a healing-potion route in the recovered loot graph.

Ghidra's `PetDog::quaff` creates `STATUS_HEALING` with duration 300 and effect
parameter 0.02 for the healing potion. `StatusEffect::step` shows that healing
is probabilistic and emits a heal effect; this is not sufficient evidence for
a deterministic six-HP grant or for the player's potion input mapping. The
runtime currently has no item-use action or status-effect model, and Circle is
only mapped to menu back outside gameplay. No potion or transition-heal
behavior was added based on this incomplete evidence. A correct next step must
recover player inventory input and the healing status path (including its
random cadence), rather than add a guaranteed heal shortcut. Host campaign and
Vita3K validation remain unpassed; the emulator still fails before the game
loads.

## Integrated-chain rerun (2026-10-18)

Re-ran `test_act_one_campaign_chain_windows.ps1` on the current proved Worm
fixture and the stored three-floor routes, selecting Rogue (class 1). The
harness confirms the Dragon is defeated on floor 0 and preserves boss mask 2,
but Rogue ends at 10/80 HP with 26 arrows. On floor 1 it reaches the Worm
without attacking it, takes Worm/spit/bat contact, and dies at tick 1,526 with
the Worm still at 100 HP. The trace has no successful ranged damage at the
Worm. The campaign therefore fails before floor 2; standalone floor proofs do
not establish a viable carried-state run. Report and full contact attribution:
`_local/host-chain-rerun-rogue-20261018/report.json` and `run.log`.

The same fixture with class 0 is also invalid as a chain route: it reaches the
floor-0 goal with the Dragon alive at 32.5 HP and the harness rejects the exit.
This demonstrates the boss gate in the validation harness is necessary. The
next route work must preserve more HP on floor 0 and solve Worm platform/terrain
aiming on floor 1 before a complete ACT_ONE campaign proof can pass. Vita3K
remains unvalidated because the isolated emulator crashes before title/game
load.

## Route regeneration and class comparison (2026-10-18)

Regenerated the three physical controller traces with the current route probe
for Rogue (class 1), Knight (class 0), and Mage (class 2), then replayed Rogue
through the integrated carried-state harness. Rogue's standalone traces defeat
Dragon (10/80 HP), Worm (2/80 HP), and Goblin King (45/80 HP), but the campaign
chain still fails on floor 1: replaying the floor-1 trace with the carried
10 HP dies at tick 870 before its first Worm hit. The fuller standalone Worm
route first damages the boss after tick 900, so the gap is exposure time, not a
missing boss hitbox or malformed packet.

Knight regenerates a floor-0 route that defeats Dragon with 25/100 HP, but the
regenerated floor-1 route dies at tick 1,591 with the Worm at 15.5 HP shortly
before its next damage sequence. Mage's floor-0 probe dies at tick 876 with the
Dragon at 63 HP; its floor-1 probe dies at tick 1,412 with Worm at 90 HP, and
its floor-2 probe stalls at the Goblin King. No class currently passes the
three-floor chain. The regenerated Rogue route artifacts are under
`_local/route-regenerated-20261018/`; class comparison artifacts are under
`_local/route-class0-regenerated-20261018/` and
`_local/route-class2-regenerated-20261018/`. All are host simulations; the VPK
has not been validated in Vita3K because its isolated instance still crashes
before loading the game.

## Worm-first route experiment (2026-10-18)

Changed the route probe's Worm policy to build a physical spawn-to-Worm path,
then a Worm-to-exit path, instead of following the exit corridor until it
happened to pass near the boss. The new trace defeats the Worm standalone at
tick 1,625 with 11/80 HP (previous route: tick 1,660 with 2/80 HP), while also
killing four common enemies. Dragon and Goblin King proofs remain unchanged and
pass under the regenerated run. Artifacts are in
`_local/route-worm-first-20261018/`.

The integrated Rogue chain still fails on floor 1. Starting with carried 10/80
HP, the player begins descending beside the Worm at about tick 840 and is hit
by Worm segments and a bat before the first boss damage; death occurs at tick
918. The position trace is `floor1-detail.csv`, and the failing chain report is
`_local/host-chain-worm-first-20261018/report.json`. This route change improves
standalone floor survival but does not yet make the persistent campaign viable.
The full Windows host suite passes (13/13) in
`_local/host-tests-worm-first-20261018/report.json`. Vita3K remains unverified;
its isolated process still fails before the game loads.

## Contact semantics and bow cadence check (2026-10-18)

Read-only comparison with Ghidra's `BossWorm::doCollision` (`00531d00.c`)
confirms its contact damage is `baseDamage - 1 + random(0..2)` and that the
level stores a 30-frame cooldown keyed by the struck Unit. The current runtime
uses the same 2–4 range and a 30-tick cooldown on the Worm entity, shared by
head and linked segments. The observed multi-source hits can still stack when
another enemy contacts the player in the same tick; no global player
invulnerability window is established by this Worm routine.

Tried shortening the route probe's Worm bow cadence from a 150-tick period to
90 ticks. The regenerated floor-1 result was identical: one arrow fired, zero
arrows hit, Worm defeated at tick 1,625 with 11 HP. The cadence edit was
reverted because it had no measurable effect. Continue with the tick-level
platform descent and aim geometry; do not add a blanket contact cooldown based
on this evidence. The Worm-first route improvement and its integrated-chain
failure remain as documented above.

## Knight plus Worm-first chain check (2026-10-18)

Regenerated all three routes with the Knight (class 0) and the new Worm-first
policy. Floor 0 defeats the Dragon at 25/100 HP, but the standalone floor-1
route dies at tick 1,936 with 7 HP while the Worm still has 35 HP; it cannot
produce a complete campaign trace set, so no Knight chain replay was possible.
Floor 2 independently proves, but does not affect the failure. This confirms
that the two partial improvements (Knight's extra Dragon HP and Worm-first
ordering) do not combine into a viable campaign route. Details are in
`_local/route-knight-worm-first-20261018/route-results.json` and
`floor-01.log`.
