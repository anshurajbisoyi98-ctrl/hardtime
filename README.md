# Prison Life — Blackwater

My native macOS prison-life game, built in C++20.

Double-click **dist/Prison Life.app** or **dist/Play Prison Life.command**.
The app is built for Apple silicon with a macOS 13.3 deployment target; only this Mac was tested.
It is a local development build.

From Terminal in this project:

```sh
./build/prison_life
```

From the delivered package:

```sh
"./dist/Prison Life.app/Contents/MacOS/PrisonLife"
```

Enter or E skips the courtroom/escort. Escape pauses or closes a menu.

## Controls

| Control | Action |
|---|---|
| WASD / arrows | Move |
| Shift | Run using agility |
| E | Interact; hold for digging |
| Space | Punch / knife / bat attack, or use selected consumable |
| 1 / 2 / 3 | Select carried item |
| G | Drop selected item with confirmation |
| M | Compound map |
| V | ElevenLabs audio showcase |
| Esc | Pause / close menu |
| Mouse / arrows + Enter | Menu selection |

Space performs one action based on the selected slot: empty hands punch, a knife or bat
attacks, and a medkit, adrenaline or vending snack is consumed. Utility items cannot attack.
Cafeteria meals still require a free seat and E; other tools use their E interactions.
Q has no use action. Pressing Space during a timed activity only cancels that activity.

## Gameplay

Start with seven sentence days, $0 and 10 HP. Meals restore sanity, never health.
One full 24-hour game day takes 5 minutes of active play: 4.8 game minutes per real second,
at the same rate during daytime and curfew. Menus pause the clock; resting can still skip
to morning. Meals and curfew use the same game hours; activity durations use real seconds.
Clean the hall spill, return the dropped book and help serve meals to earn money/conduct.
Three conduct points remove a sentence day, at most once each day. Read at library shelves;
talk with inmates for reputation and a clue. Sanctioned ring bouts pay $30–$50 and use punches.
The groundskeeper lends a task shovel at the yard shed. The dealer offers a purchase route.
Return to cell A-01 at 22:00; E beside your bunk rests until morning without restoring health.
Serve the sentence for release, or use a shovel at the persistent loose-tile hotspot for escape.

### Escape route

Carrying a purchased or loaned shovel triggers repeating prisoner ambushes in groups of one,
two and three, regardless of reputation. During this phase, replacements respawn 1.5 times faster
(about 6.7 active seconds). Reach the marked digging cell with the shovel anywhere in your inventory:
prisoners retreat and all three guards start rounds around that cell. Guard sight respects facing,
distance and walls; the shaded cones show their view and the HUD shows WATCHED or UNSEEN.
Hold E for eight uninterrupted seconds, release it, then repeat for **five completed holds**.
Releasing E or moving cancels the current partial hold; completed holds remain. Being watched
while digging immediately starts a completely fresh run and replaces the old checkpoint.
Wait while watched, and release E before a guard turns toward you. Removing the shovel ends
this special phase; carrying it back into the marked cell activates patrols again.

### Cafeteria, library and vending

At the continuous cafeteria counter, stand beside the food you want and press E to collect
that food directly. Meals still follow the same meal windows and require a free cafeteria
seat to eat. The JOB marker at the end of the counter starts the serving shift with E.

The library has two small round tables between the shelf rows, aligned with columns 2 and 4.
Each has four chairs in two opposing pairs. Shelf access and the central aisle remain open.

Vending machines charge $5 for a random soda or chips. You need a free inventory slot;
the snack is selected and held when purchased. Press Space to consume it anywhere, at any time,
for +10 sanity (capped at 100). Cafeteria meals still require a free seat.

## Saving

Day-start checkpoints are versioned JSON written atomically. The build saves beside the executable
in `build/saves`; the packaged app saves in `dist/saves`. Initial arrival and each 07:00 write a checkpoint.
Retry restores that checkpoint, rolling back money, items, stats, sentence, daily flags, resident
progress, seeded RNG/hotspot and dropped items. Positions/patrols restart safely. This is **day-start
persistence**, not continuous autosave. Pause → Load day-start checkpoint restores it; CLI `--continue`
also loads it. New game starts a fresh sentence.

## Building

Build using the project-local tools already installed:

```sh
./.tools/cmake/data/bin/cmake -S . -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$PWD/.tools/bin/ninja" -DCMAKE_BUILD_TYPE=Release
./.tools/cmake/data/bin/cmake --build build --target prison_life prison_tests prison_verify -j 8
./build/prison_tests
python3 tools/package.py
```

Recreate audio with `python3 tools/gen_assets.py`.

`prison_verify` is a separate developer executable with scripted fixtures/autoplay. No grants,
fixture relocation or bot controls are linked into the normal app. See docs/VERIFICATION.md.
Requirement status, actual evidence, limits and stage reports are under docs/.

## Testing status

Version 0.3.0 checks: 53 game tests / 1347 assertions, plus 4 offline API-client tests passed.
These cover all 12 guard patrol layouts, safe digging windows, ambush group sizes, respawn speed,
interrupted holds, five separate holds, directional visibility and restarting the disk checkpoint.
The ElevenLabs integration previously returned HTTP 200; this gameplay update makes no extra API calls.
Earlier ending/route evidence in docs/ predates the five-hold escape change unless explicitly
labelled version 0.3.0.

Prototype limits: day-start saves, simple NPC visit schedules, and short pool/basketball interactions.
A full manual seven-day playthrough and manual audio audibility were not checked. The computer-control
service could not attach to the game; native renderer capture and pointer-input tests worked.

## ElevenLabs showcase

Double-click `dist/Show ElevenLabs Demo.command`, or press **V** in the game.
The left side plays five ready-made ElevenLabs recordings without network calls or API credits.
The game also has automatic voiced verdict, arrival, meals, curfew and endings, a voiced phone
call using E at the common-hall phone, a generated corridor ambience loop, and a generated door sound.
Subtitles accompany speech. The panel's Audio toggle mutes this feature.

For the judges, click **Generate live roll call**. This makes one real Text to Speech API request
using the current day and remaining sentence. The game stays responsive, shows the HTTP result
and reported credit cost, caches the generated WAV, and plays it with subtitles. Repeated clicks
while a request is running are ignored. Generation uses account credits; demo playback does not.
The 24-hour clock pauses while the showcase panel is open.

The local key remains in `.env.local` in the project root, excluded from Git and both ZIPs.
Live requests look for `.env.local` beside the app/launcher or one folder above, or use the
`ELEVENLABS_API_KEY` environment variable. The key is never displayed, passed in process arguments,
or stored in the game binary. Live generation uses macOS `/usr/bin/python3` and `afconvert`;
offline recordings work without either a key or internet. Music API access is unavailable on
the current plan, so this feature uses Text to Speech and Sound Effects.

API bridge: `assets/elevenlabs/client.py`. Native integration: `src/broadcast.cpp`.
Recording scripts and API generation receipts: `assets/elevenlabs/lines.json` and `provenance.json`.
See `docs/ELEVENLABS_SHOWCASE.md` for a short presentation sequence and verification evidence.

## Contributors

<!-- Add contributors here -->
