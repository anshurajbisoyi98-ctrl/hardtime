"""Original procedural audio. Deterministic, standard-library-only generator."""
from pathlib import Path
import json, math, random, struct, wave

root = Path(__file__).resolve().parents[1] / 'assets' / 'sfx'
root.mkdir(parents=True, exist_ok=True)
rng = random.Random(7319)
for name, duration, frequency in [('gavel', .24, 155), ('punch', .10, 85), ('bell', .65, 660)]:
    data = bytearray()
    for i in range(int(22050 * duration)):
        t = i / 22050
        envelope = math.exp(-t * (19 if name != 'bell' else 7))
        tone = math.sin(2 * math.pi * frequency * t) + .3 * math.sin(2 * math.pi * frequency * 2.73 * t)
        noise = rng.uniform(-1, 1) * (.75 if name != 'bell' else .02)
        value = max(-1, min(1, (tone * .44 + noise * .35) * envelope))
        data.extend(struct.pack('<h', int(value * 25000)))
    with wave.open(str(root / f'{name}.wav'), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(22050)
        out.writeframes(data)
(root / 'prison.sounds.json').write_text(json.dumps({'groups': {
    name: {'file': f'{name}.wav', 'volume': .6, 'pitch_jitter': 0}
    for name in ['gavel', 'punch', 'bell']
}}, indent=2) + '\n')
print('Generated three original WAV effects and sound bank.')
