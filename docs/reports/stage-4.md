# Stage 4 — story and outcomes

Elapsed approximately 36 minutes / 84 remaining. Deadline 08:09:18 UTC.
Run `build/prison_life`; Enter skips the intro without duplicating actors.
Actual inspected frames: ../captures/stage-4-court.png and ../captures/stage-4-escape.png.

| Check | Result |
|---|---|
| Court fades/pan, verdict, seven-day sentence, synthesized gavel | Implemented; native verdict frame inspected |
| Court → collision-aware escort → cell door → officer entrance patrol | PASS, 2400-tick scenario |
| Curfew boundary/grace/witness/confront/confiscation/one incident | PASS, integrated tests |
| 07:00 reset/decrement, rest without healing, release | PASS |
| Eight-second held shovel interaction, interruption, actual escape ending | PASS |
| Versioned atomic save / complete model round trip / retry | PASS |
| Restart preserves exact roster and rolls back rewards | PASS |

Release fixture completed at tick 93 with 6 HP unchanged; escape fixture completed at tick 483.
Fixtures accelerate setup in the separate prison_verify binary. They are not a claim of a manual
seven-day playthrough. Release opens the gate in a short scene; escape shows a tunnel/daylight scene.
Day-start persistence includes all model fields, seeded hotspot/RNG, carry units, daily flags and
resident progress; positions/patrols restart safely. Continuous mid-session autosave is not implemented.

Feature freeze: final package launch, paths, regression results, documentation and requirement audit.
No major gameplay omission identified. Desktop-control smoke test remains unavailable; renderer and
engine input tests supply actual visible/input evidence. Credit usage unavailable.
