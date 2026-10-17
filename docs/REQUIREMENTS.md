# Prison Life requirement ledger

V = verified by focused tests, actual-input scenarios, or inspected native frames as specified.
I = implemented and code inspected; the precise visual/action has not received a separate manual check.
L = an explicit delivery/testing limit. Evidence and reproduction commands are in VERIFICATION.md.

| Requirement | State | Evidence / interpretation |
|---|---|---|
| C++20, supplied LambdaEngine2D, native macOS, separate lam_add_game target | V | Native builds/runs; engine revision a9d1805; original engine clean |
| Reuse ECS, named input, top-down motor, collision, camera, UI, pathfinding, saves, audio | V | Engine APIs used; five relevant engine suites pass |
| No engine replacement, web/Android conversion, publishing or paid services | V | Local desktop project, one agent, original assets |
| Court: 3-second fade, gentle pan, officer left, player right, judge top | V | Inspected stage-4-court.png; scripted court/escort run |
| Hold, verdict, one-second delayed guilty statement, seven-day sentence, gavel | V | Timed code and inspected verdict frame; WAV bank loads |
| Court fade to prison, escort to A-01, door closes, officer walks to entrance patrol | V | 2400-tick normal intro/escort scenario; collision-aware route |
| Intro skips cleanly; restart/new game has no duplicate roster/rewards | I | Skip runs verified; restart reuses the same fourteen actors and resets model |
| Square connected compound, cells north, cafe west, hall center, library east, yard south | V | Native overview + full-zone route bot |
| Four northern blocks in 2x2, three cells each, exactly twelve cells | V | Map count checks; 2 rows of 2 groups, each group 3 cells |
| All twelve cells: toilet north, bed west, passable entrance | V | Furniture counts and every interaction/hotspot collider/A* check |
| Player assigned cell, ten inmate residents, one spare; no duplicate residents | V | Stable slots: player 0, inmates cells 1-10, spare 11 |
| Cell doors/furniture/shelves/walls block movement | V | Tile collision and wall-running tests; player door toggles collision |
| WASD/arrows, normalized diagonals, Shift 90/150 movement | V | Real input/motor tests; route bots traverse all rooms |
| Agility drains only on moving runs, delayed refill, exhaustion/walk/re-enable | V | Vitals logic and wall-running test |
| Follow camera smoothing and map-bounds clamp | V | Engine CameraFollow/Bounds; inspected native frames |
| Idle/walk/facing and hit/attack feedback; running movement state | I | Procedural actor animation, engine motor velocity, brief attack/flash states |
| Orange inmates, blue officers, distinctive black dealer, hair/palette/labels | V | Native library/combat frames; dealer independent of skin palette |
| One-second outward wing overview, aligned YOU marker, no standing-doorway retrigger | V | Crossing test and inspected overview; M reopens map |
| HUD health, yellow sanity below, intelligence/agility, money/cap, rep, sentence/clock/curfew | V | Inspected world/package frames |
| Current objective, three inventory slots and nearby interaction prompts | V | Native frames; objective reacts to jobs, tools, books, curfew and clue |
| Hall phone east of northern entry, NE vending, NW pool table, clear central space | V | Map coordinates/counts, A* and actual route |
| Phone call and pool shot interactions | I | Timed actions; cord/cue-ball animation; no full pool simulation |
| Cafe: two round tables with four equally spaced circular seats each | V | Count/geometry tests; native cafe assets |
| Four distinct foods/icons, rice +20, sandwich +15, apple +10, soup +20 sanity | V | Model tests; current illustrated menu |
| Pick one meal, show in hand, carry to free seat, consume only on completion | V | Actual-input daily route; interrupted meal preserves item |
| No eating through walls or in an NPC-occupied seat | V | LOS/occupancy checks in interaction path; collision route checks |
| One free meal/window, 07-09 / 12-14 / 18-20, closed feedback, no duplicate pickup | V | Meal model/UI tests and actual route |
| Hall and cafe vending: pickup button, seeded soda/chips, $5, +10 sanity, immediate | V | Vending tests, full bag allowed, insufficient funds unchanged |
| Food/vending never heal HP | V | Focused stat checks; no HP changes in food methods |
| Library exactly 8 shelves in 2 rows x 4 columns, usable aisles | V | Counts, collision checks and navigation |
| Dealer between columns 3/4 in row 2, does not block aisle | V | Actual world location (1120,784), collider/route checks |
| Read 10 continuous active seconds, cancel movement/damage/modal interruptions | V | Integrated continuous-read/cancel tests |
| +10 intelligence, cap 100, 3 books/day, fourth rejected, reset once at 07:00 | V | Read/morning tests and daily route |
| Intelligence 20 clearer clue; escape also solvable without intelligence grinding | V | Clue code; natural escape route succeeds at starting intelligence |
| Southwest basketball, east ring, loose bat/ball props | V | Map/route; held item renderer |
| Basketball short shot animation, pickup play object, bat 2 damage/carry slot/cooldown | I | Procedural ball arc and inventory/melee rules; bat damage tested |
| Exactly 1 player, 10 inmates incl. dealer, 3 officers; court actors separate | V | ECS counted in every summary; defeat/retry retain 14 actors |
| Nine ordinary inmates route/visit/pause, dealer stationary, three officers patrol | V | 1800-tick collision-safe roaming test; dealer recovery returns to service spot |
| Fictional names, talk / support / leave, helped indicator | V | Menus and model; dealer also has talk/support actions |
| Each identity/slot +2 reputation once, cap 12, no replacement farming | V | Reputation and dealer menu tests; helped flag persists through replacement/save |
| Ambient p=.05*(1-rep/12), entry only, 30-second cooldown, 5-second grace | I | Entry/cooldown code; pure probability check; no per-frame roll |
| Rep 12 disables spontaneous aggression; player aggression permits self-defense | V | Probability and combat/roaming tests |
| Suppress ambient fights in modals, reading/meals/cutscenes and ring bouts | V | Simulation gates; modal/timer and interaction tests |
| Player HP 10, inmate HP 5, punch 1 / knife 3 / bat 2 | V | Focused damage tests |
| Melee range/cooldown, directional one-hit attack, visible hit/telegraph, 0.5s immunity | V | Player/NPC direction tests and simultaneous strike test |
| Defeat removes active service/fighting; 10-second seeded replacement, same roster slot | V | Replacement test; seeded appearance/name; ordinary assault pays nothing |
| Safe non-overlapping replacement, including a player camping dealer spot | V | Dedicated dealer recovery test and validated spawn search |
| Infirmary at zero HP, retry rolls back day-start checkpoint, no passive healing | V | Recovery/checkpoint test and no-regeneration code paths |
| Sanity -1 per 10 active seconds; zero sanity -1 HP per 10 seconds, reset on recovery | V | Multiple-frame-rate tests; independent of game clock |
| Three carried units incl. food/play/book/tool, selected equipment not extra storage | V | Inventory, transaction, meal and drop tests |
| Dealer four image/name/effect/price/buy cards, money/free slots/feedback, Esc/X close | V | Inspected current dealer menu; UiTree pointer purchase test |
| Knife $100 / shovel $400 / medkit $50 / adrenaline $50 | V | cfg/prices, cards and purchase tests; repeated $50 knife interpreted as medkit |
| Medkit +5 HP cap, adrenaline +2 HP/+2 sanity cap; do not waste when fully restored | V | Focused consumable tests including full-health medkit/full-stats adrenaline |
| Atomic buys, no negative balance, $500 cap, actual capped awards displayed | V | Capacity/funds/cap tests; jobs and ring display actual credited amount |
| Consumables disappear only on valid use; no stacking or silent item deletion | V | Model and drop/recover/full-capacity tests |
| Confirmed drop at safe nearby position, recoverable during session | V | Integrated drop test; drops included in day-start model save |
| Ring entry trigger/offer, Easy/Medium/Hard then confirmation, one existing opponent | V | Ring tests and native combat frame |
| Ring cadence 1.4/1.0/.75 seconds, telegraphed one-damage strikes, punches only | V | Config/strike tests; weapon refusal and actual-input bout |
| One seeded $30-$50 victory award, no misconduct, no replayed reward, 30s cooldown | V | Ring test; scripted bout $50, HP8, sentence unchanged |
| Loss follows recovery; leaving ring cancels, no extra opponent spawn | I | Same infirmary path and cancelBout state cleanup |
| Daily spill +$20, book return +$15, serving +$20, each +1 conduct once/day | V | Actual-input daily route and job anti-farming tests |
| Groundskeeper loaned shovel, marked garden task, return for $20/+1 and item removed | V | Full daily route; loan outside task is detectable misconduct |
| Three conduct removes one day, max one bonus/day | V | Jobs/bonus tests; route sentence 7 to 6 |
| Witnessed assault/digging/refused curfew; range+LOS warning/chase/confront | V | Integrated misconduct/curfew and ongoing-dig witness tests |
| +1 sentence distinct incident, max +2/day; confiscate implicated item; cell return | V | Integrated confiscation test, distinct incident and cap model checks |
| No extra money/reputation deduction | V | Penalty method changes sentence/incident state only |
| Clock starts 07:00 after arrival; warn21:30, curfew22:00-07:00, services close/routes open | V | Schedule tests, curfew test and navigation |
| Night clock 20x; combat/read/respawn/sanity/stamina/animation use active real time | V | Separate timers; exact clock-boundary split and pause tests |
| Rest at own bunk, brief fade, morning decrement/reset once; arrival not a served day | V | Rest/release test; HP remains 6 |
| Atomic versioned local checkpoint at arrival/day start; invalid load leaves game unchanged | V | SaveSlots, strict decode, round trip and packaged --continue run |
| Persist stats, money, inventory, sentence/conduct, daily flags, residents, seed/hotspot/RNG/drops | V | Canonical full model round trip and retry assertions |
| Mid-session continuous autosave | L | Optional; not implemented. Only day-start persistence is claimed |
| Reachable seeded cell hotspot, persistent save, nearby inspect/clue, shovel, 8s held dig | V | All hotspots checked; interruption and exact dig tests |
| Abstract tunnel/fade escape and lawful release, new game | V | Both native ending frames; real interactions drive ending states |
| End-to-end route from normal starting inventory without grants | V | Natural escape: borrowed tool, one confiscation/retry, escape tick5211, HP10, $0 |
| Source, runnable app, assets, notices, config, run guide, reports and honest checklist | V | Workspace plus dist app; package launched from /private/tmp |
| Human manual seven-day playthrough / direct desktop keyboard smoke | L | NOT RUN: desktop-control service could not attach (cgWindowNotFound) |
| Native graphics / real engine pointer-input checks | V | Inspected captured frames and UiTree raw pointer/keyboard tests |
| Audio audibility and measured performance on other hosts | L | Assets/device loading verified; not manually heard, benchmarked or cross-platform tested |

The pasted Version 2 specification is preserved as BUILD_SPEC.md. No separate handwritten map image
was supplied; written cardinal directions, normalized room allocation and exact counts determine the map.
All art/sound is original procedural work or the engine's built-in font. Pool/basketball remain short
interactions and NPC schedules are intentionally simple, as allowed by the specification.
