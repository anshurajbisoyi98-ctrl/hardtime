# Stage 0 — baseline

Elapsed 6 minutes / 114 remaining. Start 06:09:18 UTC; deadline 08:09:18 UTC.

Engine a9d1805b7f8a18ad987d38fb57b775ebfa3c3300, MIT; original tree unchanged.
Local CMake 4.4.4 and Ninja 1.13.2; pinned raylib/json/doctest downloaded and hash checked by engine.

Build PASS: `.tools/cmake/data/bin/cmake --build build --target lantern_run prison_life -j 8`.
Starter headless PASS: 600 ticks, hash be4d901dcd8bc950.
Visible native run PASS: `build/LambdaEngine2D/examples/games/03_lantern_run/lantern_run --bot --step --frames 120 --screenshot docs/captures/stage-0-starter.png`.
Screenshot captured and inspected: ../captures/stage-0-starter.png.

Resolved defaults: knife $100, shovel $400, medkit $50, adrenaline $50; dealer included in ten inmates;
four northern blocks; agility is stamina; seven sentence days; initial $0 and 10/80/0/100 HP/sanity/intelligence/agility.
Gameplay requirements in ../REQUIREMENTS.md. Credit usage unavailable.

Risk: extensive gameplay scope. First prioritize connected world and end-to-end routes, then verification.
Next: compile prison map, inspect follow camera/HUD and overview, validate map counts and reachability.
