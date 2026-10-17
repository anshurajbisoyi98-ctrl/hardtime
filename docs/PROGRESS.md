# Prison Life — saved handoff

Status: playable prototype delivered; all six stage reports saved.
Start: 2026-10-17 06:09:18 UTC / 11:39:18 IST.
Original deadline: 2026-10-17 08:09:18 UTC / 13:39:18 IST (never reset).
Handoff timestamp: 2026-10-17T06:58:40.623620+00:00.
Elapsed: 49.4 minutes. Brief user pause/continue preserved the original deadline.

Engine: a9d1805b7f8a18ad987d38fb57b775ebfa3c3300, MIT, original tree unchanged.
Built native arm64 game: build/prison_life.
Packaged app: dist/Prison Life.app. Launcher: dist/Play Prison Life.command.
Checks: 27 cases / 692 assertions; original engine physics/input/tilemap/scene/UI 5/5 PASS.
Native package launched from /private/tmp with bundled assets. Checkpoint continuation verified.
Natural escape route succeeded from normal inventory: tick5211, HP10, $0, sentence8.
Daily route: $75, INT10, one book, sentence6, all four jobs complete.

Source in src/, tests/, tools/, assets/. Full specification in docs/BUILD_SPEC.md.
Exact commands/controls in README_RUN.md; scope/evidence in docs/REQUIREMENTS.md and VERIFICATION.md.
Every stage report, actual inspected capture, and terminal log is preserved under docs/.

Known limits: day-start persistence; simple NPC schedules and short pool/basketball interactions.
No human full-week playthrough or manual audio check. Desktop-control service could not attach.
Credit usage unavailable. One agent; no paid generation, assets or external services.

Resume from this saved state for any new requested changes. Do not restart or overwrite the engine.


## Previous update — ElevenLabs showcase (17 October 2026)

- Delivered version 0.2.0 with eight generated voice recordings, corridor ambience, and door sound.
- V or Pause > ElevenLabs audio showcase opens offline demos and a real live roll-call button.
- Real native integration test: one API request, HTTP 200, generated audio loaded and narration completed.
- 46 game tests / 1186 assertions and four offline API-client tests pass.
- Packaged app launches with the existing checkpoint and in offline-only mode.
- Private key remains in the root .env.local; packages are scanned to exclude both the file and key value.
- Presentation steps and proof: docs/ELEVENLABS_SHOWCASE.md and docs/captures/elevenlabs-live.png.
- No music generation is implemented because the current account does not support Music API access.


## Latest update — shovel ambushes and stealth escape (version 0.3.0)

- Purchased and loaned shovels trigger one-, two-, and three-prisoner waves; the dealer stays a service NPC.
- Ambush-phase respawn rate is 1.5x (10 / 1.5 = about 6.7 active seconds).
- Enter the seeded digging cell with a shovel in inventory to make prisoners retreat and all three guards patrol it.
- Guard vision uses facing, 220-unit range and wall occlusion. Clipped vision cones and WATCHED/UNSEEN indicators expose it.
- Five distinct eight-second E holds open the escape; partial holds cancel on movement/release. Completed digs persist in day-start checkpoints.
- Being seen during digging restarts the whole run, including the saved checkpoint. Merely standing in view is safe.
- 53 game cases / 1347 assertions pass; all 12 patrol layouts have safe windows of at least eight seconds, guards reach every layout, and rounds include visible checks.
- Four offline ElevenLabs-client tests pass; no new API calls or credits spent for this update.
- Engine source remains unchanged. Packaged binary links only macOS system libraries; source is required for future builds, not for running the app.
- Five-hold escape developer fixture completes at tick 4728 with HP10; reactive bot releases when watched.
- Native patrol HUD capture inspected: docs/captures/shovel-patrol.png. Isolated app copy passes native launch from /private/tmp.
- Engine folder is about 13 MB; keep it for future edits/builds, or recover it from the refreshed source ZIP.
