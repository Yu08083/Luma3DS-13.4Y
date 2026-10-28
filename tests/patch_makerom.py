import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('makerom_compression_patch', ROOT / 'tools/patch_makerom.py')
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
fixture = ROOT / 'tests/fixtures/makerom/code.c'
raw = fixture.read_bytes()
assert hashlib.sha256(raw).hexdigest() == tool.ORIGINAL_SHA256
patched = tool.patch(raw)
assert tool.patch(raw.replace(b'\n', b'\r\n')) == patched
assert tool.patch(patched) == patched
assert patched.count(tool.MODE_EXPRESSION.encode()) == 2
assert patched.replace(tool.MODE_EXPRESSION.encode(), b'BLZ_NORMAL') == raw
for changed in [raw + b' ', raw.replace(b'BLZ_NORMAL', b'BLZ_BEST', 1), patched.replace(b'MAKEROM_BLZ_BEST', b'OTHER_MODE', 1)]:
    try:
        tool.patch(changed)
        raise AssertionError('Unknown source accepted')
    except ValueError:
        pass
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
modes = {}
with tempfile.TemporaryDirectory(prefix='makerom-patch-') as directory:
    stage = Path(directory)
    source = stage / 'mode.c'
    source.write_text('#include <stdlib.h>\n#include <string.h>\n#include <stdio.h>\n#define BLZ_NORMAL 0\n#define BLZ_BEST 1\nint main(void){printf("%d\\n",' + tool.MODE_EXPRESSION + ');return 0;}\n', encoding='utf-8')
    executable = stage / ('mode.exe' if os.name == 'nt' else 'mode')
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(executable)], check=True, env=environment)
    for value in [None, '', '0', '1', '2', 'yes', '01']:
        configured = dict(environment)
        configured.pop('MAKEROM_BLZ_BEST', None)
        if value is not None:
            configured['MAKEROM_BLZ_BEST'] = value
        actual = int(subprocess.check_output([str(executable)], env=configured, text=True))
        assert actual == int(value == '1'), (value, actual)
        modes['unset' if value is None else value] = actual
    copy = stage / 'code.c'
    copy.write_bytes(raw)
    subprocess.run([sys.executable, str(ROOT / 'tools/patch_makerom.py'), str(copy)], check=True, capture_output=True)
    assert copy.read_bytes() == patched
    subprocess.run([sys.executable, str(ROOT / 'tools/patch_makerom.py'), str(copy)], check=True, capture_output=True)
    assert copy.read_bytes() == patched
    copy.write_bytes(raw + b' ')
    rejected = subprocess.run([sys.executable, str(ROOT / 'tools/patch_makerom.py'), str(copy)], capture_output=True)
    assert rejected.returncode != 0 and copy.read_bytes() == raw + b' '
makefile = (ROOT / 'Luma3DS_v13_4/sysmodules/rosalina/Makefile').read_text()
assert '$(OUTPUT).cxi: export MAKEROM_BLZ_BEST := 1' in makefile
assert 'rp_jpeg.o: CFLAGS += -O2' not in makefile
report = {'all_passed': True, 'upstream_commit': tool.UPSTREAM_COMMIT, 'upstream_url': tool.UPSTREAM_URL, 'original_sha256': tool.ORIGINAL_SHA256, 'patched_sha256': hashlib.sha256(patched).hexdigest(), 'tool_sha256': hashlib.sha256((ROOT / 'tools/patch_makerom.py').read_bytes()).hexdigest(), 'mode_cases': modes, 'unknown_source_rejected_without_write': True, 'idempotent': True, 'default_mode_preserved': True, 'only_rosalina_enables_best': True}
output = ROOT / 'build/makerom-patch-tests'
output.mkdir(parents=True, exist_ok=True)
(output / 'proof.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report))
