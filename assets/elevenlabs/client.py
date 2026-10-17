"""ElevenLabs bridge. Key stays in a local file/environment, never in the app or argv."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import uuid
import wave
from datetime import datetime, timezone
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

VOICE = 'JBFqnCBsd6RMkjVDRZzb'
MODEL = 'eleven_flash_v2_5'

class AudioError(Exception):
    def __init__(self, message, status=0):
        super().__init__(message)
        self.status = status

def load_key(path):
    key = os.environ.get('ELEVENLABS_API_KEY', '').strip()
    if not key and Path(path).is_file():
        values = [line.strip().split('=', 1)[1].strip().strip('\"\u0027')
                  for line in Path(path).read_text().splitlines()
                  if line.strip().startswith('ELEVENLABS_API_KEY=')]
        if len(values) == 1:
            key = values[0]
    if not key or any(c.isspace() for c in key) or key == 'YOUR_ACTUAL_KEY_HERE':
        raise AudioError('Local API key is missing or invalid. Offline recordings are available.')
    return key

def request_audio(key, endpoint, payload):
    req = Request('https://api.elevenlabs.io' + endpoint,
                  data=json.dumps(payload).encode(),
                  headers={'xi-api-key':key, 'Content-Type':'application/json', 'Accept':'audio/mpeg'},
                  method='POST')
    try:
        with urlopen(req, timeout=40) as response:
            data = response.read(8 * 1024 * 1024)
            if response.status != 200 or len(data) < 100 or response.headers.get_content_type() != 'audio/mpeg':
                raise AudioError('The API did not return valid audio.', response.status)
            cost = response.headers.get('character-cost', '')
            metadata = {'provider':'ElevenLabs', 'http_status':response.status,
                        'generated_at_utc':datetime.now(timezone.utc).isoformat(),
                        'endpoint':endpoint.split('?')[0], 'audio_sha256':hashlib.sha256(data).hexdigest()}
            if cost.replace('.', '', 1).isdigit(): metadata['credits'] = float(cost)
            return data, metadata
    except HTTPError as error:
        friendly = {401:'Key or endpoint permission rejected.', 402:'This feature needs a different ElevenLabs plan.',
                    429:'ElevenLabs usage or request limit reached. Try later.'}
        raise AudioError(friendly.get(error.code, 'ElevenLabs request failed.') + ' Offline recordings remain available.', error.code) from None
    except (URLError, OSError, TimeoutError):
        raise AudioError('ElevenLabs is unreachable. Offline recordings remain available.') from None

def speech(key, text):
    data, metadata = request_audio(key, '/v1/text-to-speech/' + VOICE + '?output_format=mp3_44100_128',
                                  {'text':text, 'model_id':MODEL})
    metadata.update(model=MODEL, voice_id=VOICE, text=text)
    return data, metadata

def sound_effect(key, text, seconds=6):
    data, metadata = request_audio(key, '/v1/sound-generation?output_format=mp3_44100_128',
                                  {'text':text, 'duration_seconds':seconds, 'loop':True,
                                   'model_id':'eleven_text_to_sound_v2'})
    metadata.update(model='eleven_text_to_sound_v2', text=text)
    return data, metadata

def decode_audio(data, destination):
    """Convert API MP3 to engine-ready WAV using the macOS system decoder."""
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='blackwater-audio-') as work:
        source = Path(work) / 'input.mp3'
        source.write_bytes(data)
        decoded = Path(work) / 'decoded.wav'
        result = subprocess.run(['/usr/bin/afconvert', '-f', 'WAVE', '-d', 'LEI16', str(source), str(decoded)],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=15)
        if result.returncode or not decoded.is_file():
            raise AudioError('Audio decoding failed. Offline recordings are available.')
        with wave.open(str(decoded), 'rb') as audio:
            seconds = audio.getnframes() / audio.getframerate()
            if not 0 < seconds <= 60: raise AudioError('Generated audio duration is invalid.')
        staging = destination.with_name(destination.name + '.' + uuid.uuid4().hex + '.tmp')
        staging.write_bytes(decoded.read_bytes())
        os.replace(staging, destination)
    return seconds

def write_json(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.' + uuid.uuid4().hex + '.tmp')
    temporary.write_text(json.dumps(data, indent=2) + '\n')
    os.replace(temporary, path)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--key-file', required=True)
    parser.add_argument('--result', required=True)
    parser.add_argument('--day', type=int, required=True)
    parser.add_argument('--sentence', type=int, required=True)
    args = parser.parse_args()
    report = {'ok':False, 'http_status':0, 'message':'Offline recordings are available.'}
    try:
        if not 1 <= args.day <= 1000000 or not 0 <= args.sentence <= 10000:
            raise AudioError('Invalid roll call state.')
        text = ('Blackwater roll call. Day ' + str(args.day) + '. Prisoner zero seven one, ' +
                str(args.sentence) + ' days remaining. Keep your head up and make today count.')
        key = load_key(args.key_file)
        data, metadata = speech(key, text)
        destination = Path(args.result).parent / ('roll-call-' + uuid.uuid4().hex + '.wav')
        duration = decode_audio(data, destination)
        report.update(metadata, ok=True, file=str(destination.resolve()), duration=duration,
                      message='Live request succeeded. HTTP 200.')
    except AudioError as error:
        report.update(message=str(error), http_status=error.status)
    except Exception:
        report['message'] = 'Audio service could not complete. Offline recordings are available.'
    write_json(args.result, report)
    print(json.dumps({'ok':report['ok'], 'http_status':report['http_status']}))
    return 0 if report['ok'] else 1

if __name__ == '__main__':
    sys.exit(main())
