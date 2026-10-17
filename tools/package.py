"""Create a local, unsigned macOS app bundle without modifying the engine."""
from pathlib import Path
import os, plistlib, shutil

root = Path(__file__).resolve().parents[1]
dist = root / 'dist'
app = dist / 'Prison Life.app'
mac = app / 'Contents' / 'MacOS'
resources = app / 'Contents' / 'Resources'
mac.mkdir(parents=True, exist_ok=True)
resources.mkdir(parents=True, exist_ok=True)
staged = mac / 'PrisonLife.next'
shutil.copy2(root / 'build' / 'prison_life', staged)
os.replace(staged, mac / 'PrisonLife')
os.chmod(mac / 'PrisonLife', 0o755)
shutil.copytree(root / 'assets', mac / 'assets', dirs_exist_ok=True,
                ignore=shutil.ignore_patterns('__pycache__', '*.pyc', '.env*'))
if (root / 'assets' / 'PrisonLife.icns').exists():
    shutil.copy2(root / 'assets' / 'PrisonLife.icns', resources / 'PrisonLife.icns')
with (app / 'Contents' / 'Info.plist').open('wb') as f:
    plistlib.dump({
        'CFBundleExecutable': 'PrisonLife', 'CFBundleIdentifier': 'dev.blackwater.prisonlife',
        'CFBundleName': 'Prison Life', 'CFBundleDisplayName': 'Prison Life',
        'CFBundlePackageType': 'APPL', 'CFBundleShortVersionString': '0.3.0',
        'CFBundleVersion': '3', 'LSMinimumSystemVersion': '13.3',
        'NSHighResolutionCapable': True, 'CFBundleIconFile': 'PrisonLife.icns',
    }, f)
launcher = dist / 'Play Prison Life.command'
launcher.write_text('#!/bin/zsh\ncd -- "$(dirname "$0")"\nexec "./Prison Life.app/Contents/MacOS/PrisonLife" "$@"\n')
os.chmod(launcher, 0o755)
showcase = dist / 'Show ElevenLabs Demo.command'
showcase.write_text('#!/bin/zsh\ncd -- "$(dirname "$0")"\nexec "./Prison Life.app/Contents/MacOS/PrisonLife" --continue --audio-showcase "$@"\n')
os.chmod(showcase, 0o755)
for name in ['README_RUN.md', 'ASSET_CREDITS.md']:
    if (root / name).exists():
        shutil.copy2(root / name, dist / name)
notices = dist / 'NOTICES'
notices.mkdir(exist_ok=True)
shutil.copy2(root / 'LambdaEngine2D' / 'LICENSE', notices / 'LambdaEngine2D-MIT.txt')
shutil.copy2(root / 'LambdaEngine2D' / 'THIRD_PARTY.md', notices / 'THIRD_PARTY.md')
for pattern, name in [('raylib-src-*/LICENSE', 'raylib-LICENSE.txt'),
                      ('nlohmann_json-src/LICENSE.MIT', 'json-LICENSE.txt'),
                      ('doctest-src/LICENSE.txt', 'doctest-LICENSE.txt')]:
    matches = list((root / 'build' / '_deps').glob(pattern))
    if matches:
        shutil.copy2(matches[0], notices / name)
print(app)
