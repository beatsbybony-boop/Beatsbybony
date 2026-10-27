"""Run after `npx cap add android`: installs the native recorder and its manifest entries."""
import pathlib, re, shutil

root = pathlib.Path('android/app/src/main')
pkg_dir = root / 'java' / 'com' / 'beatsbybony' / 'cooledit'
pkg_dir.mkdir(parents=True, exist_ok=True)
for f in pathlib.Path('native/java').glob('*.java'):
    shutil.copy(f, pkg_dir / f.name)

mf = root / 'AndroidManifest.xml'
t = mf.read_text()
perms = '''    <uses-permission android:name="android.permission.RECORD_AUDIO" />
    <uses-permission android:name="android.permission.MODIFY_AUDIO_SETTINGS" />
    <uses-permission android:name="android.permission.FOREGROUND_SERVICE" />
    <uses-permission android:name="android.permission.FOREGROUND_SERVICE_MEDIA_PROJECTION" />
    <uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
'''
svc = '''        <service android:name=".MixCaptureService" android:exported="false" android:foregroundServiceType="mediaProjection" />
'''
assert '<application' in t and '</application>' in t
t = t.replace('<application', perms + '\n    <application', 1)
t = t.replace('</application>', svc + '    </application>', 1)
mf.write_text(t)
print('patched manifest and installed native plugin')
