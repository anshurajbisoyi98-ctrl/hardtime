# ElevenLabs feature — The Voice of Blackwater

## One-minute judge demonstration

1. Open **dist/Show ElevenLabs Demo.command** or press **V** while playing.
2. Click **Play prison PA**, then **Play call from home**. These are actual ElevenLabs recordings,
   bundled so the presentation works offline. Switching clips stops the previous narration.
3. Click **Generate live roll call** once. The native game starts a nonblocking Python HTTPS
   request using the local key. When it finishes, show **HTTP 200**, request count and reported
   credit usage. The generated message names the actual in-game day and remaining sentence.
4. The voice plays with subtitles. This is a new API response, not a prerecorded success message.
5. Close the panel and use **E** at the common-hall phone to hear a family call in gameplay.

The judge-facing panel never displays the secret. Showing an actual response and generated audio
is the evidence that the API was used. The credential remains in the local `.env.local` file.

## Integrated gameplay audio

- The judge speaks during the verdict.
- Arrival, meal windows and the curfew warning have spoken announcements.
- The phone plays a family message; movement, attack interruption or damage stops the call.
- Escape and release have their own spoken endings.
- Generated corridor ambience is quieter during speech, and the cell door has a generated latch sound.
- The showcase plays five selectable clips, has subtitles, and supports muting ElevenLabs audio.
- Opening the showcase pauses gameplay and its clock. API requests run independently of rendering.

All voice lines are authored game dialogue synthesized through ElevenLabs. The live roll call
uses a state-based text template rather than an LLM. There is no claim of free-form NPC intelligence.

## Key and connection handling

- The app reads a local `.env.local` beside the app/launcher or one directory above, or inherits
  `ELEVENLABS_API_KEY` from its environment. On this project the file is at the repository root.
- The secret is sent only as an HTTPS authentication header to `api.elevenlabs.io`. It is not
  embedded in the binary, command-line arguments, screenshots, logs, public receipts or ZIPs.
- Missing keys, network errors, permission/plan limits and decoder errors produce an honest status;
  bundled recordings remain usable. No automatic retry or repeated generation runs per frame.
- A live click is billable. While one request is running, further clicks are ignored.
- Live generation uses the macOS system Python and audio decoder. Offline playback is standalone.
- Automated/headless runs cannot call the API unless explicitly given `--allow-live-audio`.

## Verification

- The native in-game button was exercised once against the real API: one request, HTTP 200,
  generated WAV loaded by the engine, completed narration, no model/gameplay state changed.
- API-reported live test cost: **56 credits**. Eight bundled lines plus ambience: **508 credits**.
  The door reuses the earlier successful half-second Sound Effects API test.
- All ten bundled WAV files decoded and contained non-silent PCM audio. Native audio device,
  sound uploads and ambience streaming initialized successfully. No claim of a separate manual
  listening review is made.
- Live result capture: `docs/captures/elevenlabs-live.png`.
- Native run log: `docs/logs/prison-elevenlabs-live.log`.
- Game regression log: `docs/logs/prison-elevenlabs-tests.log`.
- Offline client tests: `python3 tests/test_elevenlabs.py`.
- Generation provenance: `assets/elevenlabs/provenance.json`.

The current account rejects Music API calls with HTTP 402. No music feature is represented as
working; this showcase uses the working Text to Speech and Sound Effects capabilities.

Official API references:
- https://elevenlabs.io/docs/api-reference/text-to-speech/convert
- https://elevenlabs.io/docs/api-reference/text-to-sound-effects/convert
