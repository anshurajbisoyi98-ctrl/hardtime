# Stage 1 — world

Elapsed approximately 17 minutes / 103 remaining. Deadline remains 08:09:18 UTC.
Native game built and launched. Actual frames inspected: ../captures/stage-1-world-fit.png and
../captures/stage-1-map-fit.png. Window reduced to 1200x750 and Fit scaling to avoid the original
oversized native window clipping the upper HUD.

Run: `build/prison_life` (Enter skips the intro); development capture used `--skip-intro --step --frames 20`.

| Check | Result |
|---|---|
| 12 cells, 12 west beds, 12 north toilets | PASS, focused tests |
| 8 shelves, 2 round tables, 8 seats, 2 vending machines | PASS, focused tests |
| Every prop interaction, cell hotspot, dealer and ring spawn reachable | PASS, engine A* and collider checks |
| HUD, cell scale and overview visually inspected | PASS |
| Basic game headless 1800 ticks | PASS |
| Map/model focused assertions | 414 PASS |

Found and fixed collision-raster edge overlap at bed/toilet interaction points.
Movement uses TopDownMotor; collision uses TileWorld; camera uses CameraFollow/Bounds.
M map and one-second wing outward crossing overview share world coordinates.
Remaining: visible interaction/combat smoke tests and daily-life/outcome validation.
Next: NPC patrols, cooldown combat, respawn and interactive UI. Credit usage unavailable.
