"""Refresh deliverables and reject accidental inclusion of the local ElevenLabs secret."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os
import re
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, str(root / 'tools/package.py')], check=True)
dist = root / 'dist'
binary = dist / 'Prison Life.app/Contents/MacOS/PrisonLife'
digest = hashlib.sha256(binary.read_bytes()).hexdigest()
assert digest == hashlib.sha256((root / 'build/prison_life').read_bytes()).hexdigest()
log = (root / 'docs/logs/prison-shovel-tests.log').read_text()
assert 'Status: SUCCESS!' in log
cases = int(re.search(r'test cases:\s+(\d+)', log)[1])
assertions = int(re.search(r'assertions:\s+(\d+)', log)[1])
metadata_path = dist / 'BUILD_INFO.json'
metadata = json.loads(metadata_path.read_text())
metadata.update(version='0.3.0', updated_at_utc=datetime.now(timezone.utc).isoformat(),
                update='Shovel ambush waves, faster replacement inmates, three-guard stealth patrol and five-hold escape',
                game_sha256=digest, test_cases=cases, assertions=assertions, api_client_tests=4,
                shovel_escape={'required_holds':5,'seconds_per_hold':8,'ambush_group_sizes':[1,2,3],
                               'respawn_rate_multiplier':1.5,'guard_count':3,'caught_digging_restarts_run':True,
                               'all_12_patrol_layouts_tested':True,'engine_source_required_to_run':False},
                elevenlabs={'voice_recordings':8, 'generated_ambience':True, 'generated_door_effect':True,
                            'live_native_test_http':200, 'asset_generation_credits':508, 'live_test_credits':56,
                            'secret_bundled':False})
metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
key_file = root / '.env.local'
secret = None
if key_file.is_file():
    for line in key_file.read_text().splitlines():
        if line.strip().startswith('ELEVENLABS_API_KEY='):
            secret = line.split('=',1)[1].strip().strip('\"\u0027').encode()

def safe(path):
    return not any(part in {'__pycache__','.git','.DS_Store'} for part in path.parts) and not path.name.startswith('.env') and path.suffix != '.pyc'

app_members = [p.relative_to(dist).as_posix() for directory in ['Prison Life.app','NOTICES']
               for p in (dist / directory).rglob('*') if p.is_file() and safe(p.relative_to(dist))]
app_members += ['Play Prison Life.command','Show ElevenLabs Demo.command','README_RUN.md','ASSET_CREDITS.md','BUILD_INFO.json']
source_members = set()
source_archive = dist / 'PrisonLife-source.zip'
if source_archive.exists():
    with zipfile.ZipFile(source_archive) as old:
        source_members.update(name for name in old.namelist() if name.startswith('LambdaEngine2D/'))
for directory in ['src','tests','tools','assets','docs']:
    source_members.update(p.relative_to(root).as_posix() for p in (root / directory).rglob('*') if p.is_file() and safe(p.relative_to(root)))
source_members.update(['CMakeLists.txt','README_RUN.md','ASSET_CREDITS.md','.gitignore'])
for name, base, members in [('PrisonLife-macOS.zip',dist,sorted(set(app_members))),
                            ('PrisonLife-source.zip',root,sorted(source_members))]:
    for member in members:
        path = base / member
        assert path.is_file() and safe(Path(member)), 'Unsafe archive entry: ' + member
        assert not secret or secret not in path.read_bytes(), 'Secret found; refusing to package: ' + member
    staging = dist / (name + '.tmp')
    with zipfile.ZipFile(staging,'w',zipfile.ZIP_DEFLATED) as archive:
        for member in members: archive.write(base / member, member)
    with zipfile.ZipFile(staging) as archive:
        assert archive.testzip() is None
    os.replace(staging,dist / name)
    print(name + ': verified, ' + str(len(members)) + ' files, no local key')
print('Packaged app matches tested build: ' + digest)
