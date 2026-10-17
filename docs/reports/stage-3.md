# Stage 3 — daily life

Elapsed approximately 33 minutes / 87 remaining. Original deadline remains 08:09:18 UTC.
Run `build/prison_life`. Native four-card menus captured and inspected:
../captures/stage-3-dealer.png and ../captures/stage-3-food.png.

| Check | Result |
|---|---|
| Actual-input daily route: meal, cleaning, book return, read, serving, garden/return | PASS |
| Daily route rewards | $75, 10 intelligence, 1 book, sentence 6, jobs 1111 |
| Real UiTree pointer click / keyboard input capture | PASS |
| Atomic purchase, three units, insufficient funds/full capacity | PASS |
| Reading 10 seconds, interruption, three/day and morning reset | PASS |
| No passive healing; food does not heal; consumable caps | PASS |
| Sanity damage across frame rates, modal pause, actual movement stamina | PASS |
| Focused checks at this stage | 21 cases / 657 assertions PASS |

Two round tables have four circular seats each. NPC occupancy blocks eating; interrupted meals remain
carried. Vending immediately consumes a seeded snack for $5/+10 sanity. All four jobs require interaction
completion; three conduct removes one day, once/day. Reputation is awarded once per roster slot.
Drop confirmation preserves recoverable objects. Dealer and meal menus capture gameplay input.

Next: final story/curfew/endings evidence, package launch, existing module tests, requirement audit.
Direct desktop-control service unavailable; native capture and engine pointer-input tests work.
Credit usage unavailable.
