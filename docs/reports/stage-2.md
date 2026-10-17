# Stage 2 — population and combat

Elapsed approximately 26 minutes / 94 remaining. Deadline 08:09:18 UTC.
Run `build/prison_life`; current native target compiles successfully.
Actual combat frame captured and inspected: ../captures/stage-2-combat.png.

| Check | Result |
|---|---|
| Roster counted from ECS: 1 player, 10 inmates, 3 officers | PASS |
| Directional punch 1 / knife 3 / bat 2, cooldown, no modal attack leaks | PASS |
| Inmate defeated at 5 damage, one replacement after 10 active seconds, roster unchanged | PASS |
| Ring uses existing inmate, rejects weapons, pays once, no sentence penalty | PASS |
| Scripted easy bout | $50 reward, 8 HP remaining, sentence unchanged |
| Eight-zone navigation bot | PASS, visit mask 255 at tick 2895 |
| Integrated rule checks | 502 assertions PASS |

Roaming uses engine A* and TopDownMotor; dealer stays in library. Inmate encounter checks occur
on radius entry with cooldown, grace and suppression. Menu/map pause stops both clocks and timers.
Respawn retains helped status to prevent reputation farming. Replacement appearance/name uses seeded RNG.
Visible strike telegraphs and NPC health bars added after inspecting combat frame.

Next: inspect daily-life menus, execute full jobs/food/library route, finish story/save/curfew checks.
No known population blocker. Credit usage unavailable.
