"""Generate and cache ElevenLabs showcase assets without logging the API key."""
from pathlib import Path
import json
import shutil
import sys
sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'assets/elevenlabs'))
import client

key = client.load_key(root / '.env.local')
manifest = json.loads((root / 'assets/elevenlabs/lines.json').read_text())
provenance_path = root / 'assets/elevenlabs/provenance.json'
provenance = json.loads(provenance_path.read_text()) if provenance_path.exists() else {'provider':'ElevenLabs', 'clips':{}}
output = root / 'assets/sfx/elevenlabs'
output.mkdir(parents=True, exist_ok=True)
for line in manifest['lines']:
    name = line['id']
    destination = output / (name + '.wav')
    if destination.exists() and name in provenance['clips']:
        print(name + ': cached', flush=True); continue
    data, metadata = client.speech(key, line['text'])
    metadata['duration'] = client.decode_audio(data, destination)
    provenance['clips'][name] = metadata
    client.write_json(provenance_path, provenance)
    print(name + ': generated, HTTP 200, credits ' + str(metadata.get('credits', 'unreported')), flush=True)
ambient = output / 'corridor.wav'
if not ambient.exists():
    prompt = 'Quiet distant prison corridor ambience. Gentle ventilation and soft distant footsteps, no voices, no music. Seamless loop.'
    data, metadata = client.sound_effect(key, prompt)
    metadata['duration'] = client.decode_audio(data, ambient)
    provenance['clips']['corridor'] = metadata
    client.write_json(provenance_path, provenance)
    print('corridor: generated, HTTP 200, credits ' + str(metadata.get('credits', 'unreported')), flush=True)
shutil.copy2(root / 'docs/elevenlabs-tests/sound_effects.wav', output / 'door.wav')
provenance['clips']['door'] = json.loads((root / 'docs/elevenlabs-tests/sound_effects.json').read_text())
client.write_json(provenance_path, provenance)
print('All audio assets ready.', flush=True)
